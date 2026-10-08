// boot_test: run a DS ROM headless in the melonDS core with scripted input.
//
// Used to check that a modified (non-matching) build of the game still behaves
// exactly like the original: both ROMs are run with the same input script, a
// fixed clock and an empty save, and the per-frame hash of both screens is
// logged. tools/compare_boot.py diffs two such logs.
//
// Usage:
//   boot_test <rom.nds> <outdir> [--frames N] [--script inputs.txt]
//             [--shot F1,F2,...] [--shot-every N] [--no-jit]
//             [--dump-ram F1,F2,...] [--pin32 ADDR=VALUE]
//
// Input script, one command per line ('#' comments):
//   <frame> <BUTTON[+BUTTON...]> <hold_frames>     e.g.  300 START 6
//   <frame> TOUCH <x> <y> <hold_frames>            e.g.  900 TOUCH 128 96 4
// Buttons: A B X Y L R START SELECT UP DOWN LEFT RIGHT
//
// Outputs in <outdir>: hashes.txt (frame, hash, PC of ARM9), shot_<frame>.ppm,
// sram.bin (battery save at exit), ram_<frame>.bin (memory snapshot used by
// tools/difftest.py: 4 MiB main RAM, then 32 KiB ITCM, then 16 KiB DTCM).
//
// --pin32 rewrites a main-RAM word before every frame. It is a diagnostic for
// comparing builds whose code runs at different speeds: pinning the game's
// rand() state (whose seed comes from a timing-dependent counter) removes the
// one source of run-to-run divergence that isn't caused by the code itself.

#include "NDS.h"
#include "NDSCart.h"
#include "GPU.h"
#include "SPU.h"
#include "Platform.h"
#include "Args.h"
#include "RTC.h"
#include "SPI.h"

#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace melonDS;

// ---------------------------------------------------------------- Platform
static std::vector<u8> g_sram;
static bool g_stopped = false;

namespace melonDS::Platform
{
void SignalStop(StopReason reason, void*) { g_stopped = true; }

static std::string ModeString(FileMode mode, bool exists)
{
    std::string m;
    if (mode & FileMode::Append) m += 'a';
    else if (!(mode & FileMode::Write)) m += 'r';
    else if (mode & FileMode::NoCreate) m += 'r';
    else if ((mode & FileMode::Preserve) && exists) m += 'r';
    else m += 'w';
    if ((mode & FileMode::ReadWrite) == FileMode::ReadWrite) m += '+';
    if (!(mode & FileMode::Text)) m += 'b';
    return m;
}
std::string GetLocalFilePath(const std::string& f) { return f; }
FileHandle* OpenFile(const std::string& path, FileMode mode)
{
    return (FileHandle*)fopen(path.c_str(), ModeString(mode, FileExists(path)).c_str());
}
FileHandle* OpenLocalFile(const std::string& path, FileMode mode) { return nullptr; }
bool FileExists(const std::string& name) { FILE* f = fopen(name.c_str(), "rb"); if (f) fclose(f); return f != nullptr; }
bool LocalFileExists(const std::string&) { return false; }
bool CheckFileWritable(const std::string&) { return true; }
bool CheckLocalFileWritable(const std::string&) { return false; }
bool CloseFile(FileHandle* f) { return fclose((FILE*)f) == 0; }
bool IsEndOfFile(FileHandle* f) { return feof((FILE*)f) != 0; }
bool FileReadLine(char* s, int n, FileHandle* f) { return fgets(s, n, (FILE*)f) != nullptr; }
u64 FilePosition(FileHandle* f) { return ftell((FILE*)f); }
bool FileSeek(FileHandle* f, s64 off, FileSeekOrigin o)
{
    int w = o == FileSeekOrigin::Start ? SEEK_SET : o == FileSeekOrigin::Current ? SEEK_CUR : SEEK_END;
    return fseek((FILE*)f, off, w) == 0;
}
void FileRewind(FileHandle* f) { rewind((FILE*)f); }
u64 FileRead(void* d, u64 s, u64 c, FileHandle* f) { return fread(d, s, c, (FILE*)f); }
bool FileFlush(FileHandle* f) { return fflush((FILE*)f) == 0; }
u64 FileWrite(const void* d, u64 s, u64 c, FileHandle* f) { return fwrite(d, s, c, (FILE*)f); }
u64 FileWriteFormatted(FileHandle* f, const char* fmt, ...)
{
    va_list a; va_start(a, fmt); int r = vfprintf((FILE*)f, fmt, a); va_end(a); return r;
}
u64 FileLength(FileHandle* f)
{
    long p = ftell((FILE*)f); fseek((FILE*)f, 0, SEEK_END); long l = ftell((FILE*)f); fseek((FILE*)f, p, SEEK_SET); return l;
}

void Log(LogLevel level, const char* fmt, ...)
{
    if (level < LogLevel::Warn && !getenv("BOOT_TEST_VERBOSE")) return;
    va_list a; va_start(a, fmt); vfprintf(stderr, fmt, a); va_end(a);
}

struct Thread { std::thread t; };
Thread* Thread_Create(std::function<void()> fn) { auto* t = new Thread; t->t = std::thread(fn); return t; }
void Thread_Free(Thread* t) { if (t->t.joinable()) t->t.detach(); delete t; }
void Thread_Wait(Thread* t) { if (t->t.joinable()) t->t.join(); }

struct Semaphore { std::mutex m; std::condition_variable cv; int count = 0; };
Semaphore* Semaphore_Create() { return new Semaphore; }
void Semaphore_Free(Semaphore* s) { delete s; }
void Semaphore_Reset(Semaphore* s) { std::lock_guard<std::mutex> l(s->m); s->count = 0; }
void Semaphore_Wait(Semaphore* s) { std::unique_lock<std::mutex> l(s->m); s->cv.wait(l, [&] { return s->count > 0; }); s->count--; }
bool Semaphore_TryWait(Semaphore* s, int ms)
{
    std::unique_lock<std::mutex> l(s->m);
    if (!s->cv.wait_for(l, std::chrono::milliseconds(ms), [&] { return s->count > 0; })) return false;
    s->count--; return true;
}
void Semaphore_Post(Semaphore* s, int n) { { std::lock_guard<std::mutex> l(s->m); s->count += n; } s->cv.notify_all(); }

struct Mutex { std::mutex m; };
Mutex* Mutex_Create() { return new Mutex; }
void Mutex_Free(Mutex* m) { delete m; }
void Mutex_Lock(Mutex* m) { m->m.lock(); }
void Mutex_Unlock(Mutex* m) { m->m.unlock(); }
bool Mutex_TryLock(Mutex* m) { return m->m.try_lock(); }

void Sleep(u64 us) { std::this_thread::sleep_for(std::chrono::microseconds(us)); }
u64 GetMSCount() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
u64 GetUSCount() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

void WriteNDSSave(const u8* data, u32 len, u32, u32, void*) { g_sram.assign(data, data + len); }
void WriteGBASave(const u8*, u32, u32, u32, void*) {}
void WriteFirmware(const Firmware&, u32, u32, void*) {}
void WriteDateTime(int, int, int, int, int, int, void*) {}

void MP_Begin(void*) {}
void MP_End(void*) {}
int MP_SendPacket(u8*, int, u64, void*) { return 0; }
int MP_RecvPacket(u8*, u64*, void*) { return 0; }
int MP_SendCmd(u8*, int, u64, void*) { return 0; }
int MP_SendReply(u8*, int, u64, u16, void*) { return 0; }
int MP_SendAck(u8*, int, u64, void*) { return 0; }
int MP_RecvHostPacket(u8*, u64*, void*) { return 0; }
u16 MP_RecvReplies(u8*, u64, u16, void*) { return 0; }
int Net_SendPacket(u8*, int, void*) { return 0; }
int Net_RecvPacket(u8*, void*) { return 0; }

void Camera_Start(int, void*) {}
void Camera_Stop(int, void*) {}
void Camera_CaptureFrame(int, u32*, int, int, bool, void*) {}
void Mic_Start(void*) {}
void Mic_Stop(void*) {}
int Mic_ReadInput(s16* d, int n, void*) { memset(d, 0, n * 2); return n; }

AACDecoder* AAC_Init() { return nullptr; }
void AAC_DeInit(AACDecoder*) {}
bool AAC_Configure(AACDecoder*, int, int) { return false; }
bool AAC_DecodeFrame(AACDecoder*, const void*, int, void*, int) { return false; }

bool Addon_KeyDown(KeyType, void*) { return false; }
void Addon_RumbleStart(u32, void*) {}
void Addon_RumbleStop(void*) {}
float Addon_MotionQuery(MotionQueryType, void*) { return 0; }

DynamicLibrary* DynamicLibrary_Load(const char*) { return nullptr; }
void DynamicLibrary_Unload(DynamicLibrary*) {}
void* DynamicLibrary_LoadFunction(DynamicLibrary*, const char*) { return nullptr; }
}

// ---------------------------------------------------------------- harness
struct InputEvent { int frame, hold; u32 keys; bool touch; int x, y; };

static u32 ButtonBit(const std::string& n)
{
    static const std::map<std::string, int> m = {
        {"A", 0}, {"B", 1}, {"SELECT", 2}, {"START", 3}, {"RIGHT", 4}, {"LEFT", 5},
        {"UP", 6}, {"DOWN", 7}, {"R", 8}, {"L", 9}, {"X", 10}, {"Y", 11}};
    auto it = m.find(n);
    if (it == m.end()) { fprintf(stderr, "unknown button %s\n", n.c_str()); exit(2); }
    return 1u << it->second;
}

static std::vector<InputEvent> LoadScript(const std::string& path)
{
    std::vector<InputEvent> ev;
    std::ifstream in(path);
    if (!in) { fprintf(stderr, "cannot open script %s\n", path.c_str()); exit(2); }
    std::string line;
    while (std::getline(in, line))
    {
        auto h = line.find('#'); if (h != std::string::npos) line.resize(h);
        std::istringstream ss(line);
        InputEvent e{}; std::string what;
        if (!(ss >> e.frame >> what)) continue;
        if (what == "TOUCH") { e.touch = true; ss >> e.x >> e.y >> e.hold; }
        else
        {
            std::stringstream parts(what); std::string p;
            while (std::getline(parts, p, '+')) e.keys |= ButtonBit(p);
            ss >> e.hold;
        }
        if (e.hold <= 0) e.hold = 1;
        ev.push_back(e);
    }
    return ev;
}

static u64 Hash(const u32* a, const u32* b)
{
    u64 h = 1469598103934665603ull;
    for (int i = 0; i < 256 * 192; i++) { h ^= a[i] & 0xFFFFFF; h *= 1099511628211ull; }
    for (int i = 0; i < 256 * 192; i++) { h ^= b[i] & 0xFFFFFF; h *= 1099511628211ull; }
    return h;
}

static void WritePPM(const std::string& path, const u32* top, const u32* bot)
{
    FILE* f = fopen(path.c_str(), "wb");
    fprintf(f, "P6\n256 384\n255\n");
    for (int s = 0; s < 2; s++)
        for (int i = 0; i < 256 * 192; i++)
        {
            u32 c = (s ? bot : top)[i];
            u8 px[3] = {(u8)(c >> 16), (u8)(c >> 8), (u8)c};
            fwrite(px, 1, 3, f);
        }
    fclose(f);
}

int main(int argc, char** argv)
{
    if (argc < 3) { fprintf(stderr, "usage: boot_test <rom> <outdir> [--frames N] [--script f] [--shot a,b] [--shot-every N] [--no-jit]\n"); return 2; }
    std::string rompath = argv[1], outdir = argv[2];
    int frames = 3600, shotEvery = 0; bool jit = true;
    std::set<int> shots, dumps; std::vector<InputEvent> script;
    std::vector<std::pair<u32, u32>> pins;
    for (int i = 3; i < argc; i++)
    {
        std::string a = argv[i];
        if (a == "--frames") frames = atoi(argv[++i]);
        else if (a == "--script") script = LoadScript(argv[++i]);
        else if (a == "--shot") { std::stringstream ss(argv[++i]); std::string t; while (std::getline(ss, t, ',')) shots.insert(atoi(t.c_str())); }
        else if (a == "--shot-every") shotEvery = atoi(argv[++i]);
        else if (a == "--no-jit") jit = false;
        else if (a == "--pin32")
        {
            std::string v = argv[++i]; auto eq = v.find('=');
            pins.push_back({(u32)strtoul(v.substr(0, eq).c_str(), nullptr, 0), (u32)strtoul(v.substr(eq + 1).c_str(), nullptr, 0)});
        }
        else if (a == "--dump-ram") { std::stringstream ss(argv[++i]); std::string t; while (std::getline(ss, t, ',')) dumps.insert(atoi(t.c_str())); }
    }

    std::ifstream rf(rompath, std::ios::binary);
    if (!rf) { fprintf(stderr, "cannot open %s\n", rompath.c_str()); return 2; }
    std::vector<u8> rom((std::istreambuf_iterator<char>(rf)), {});

    NDSArgs args;
    if (!jit) args.JIT = std::nullopt;
    auto nds = std::make_unique<NDS>(std::move(args), nullptr);
    RendererSettings rs{1, false, false, false};
    nds->GetRenderer().SetRenderSettings(rs);

    NDSCart::NDSCartArgs cartargs{};
    auto cart = NDSCart::ParseROM(rom.data(), (u32)rom.size(), nullptr, std::move(cartargs));
    if (!cart) { fprintf(stderr, "ROM rejected by core\n"); return 1; }
    nds->SetNDSCart(std::move(cart));
    nds->Reset();
    nds->SPI.GetPowerMan()->SetBatteryLevelOkay(true);
    nds->RTC.SetDateTime(2026, 1, 1, 12, 0, 0);   // fixed clock => deterministic runs
    nds->SetupDirectBoot("game.nds");
    nds->Start();

    std::string hpath = outdir + "/hashes.txt";
    FILE* hf = fopen(hpath.c_str(), "w");
    if (!hf) { fprintf(stderr, "cannot write %s (does the directory exist?)\n", hpath.c_str()); return 2; }

    for (int fr = 0; fr < frames && !g_stopped; fr++)
    {
        u32 keys = 0; bool touch = false; int tx = 0, ty = 0;
        for (auto& e : script)
            if (fr >= e.frame && fr < e.frame + e.hold)
            {
                keys |= e.keys;
                if (e.touch) { touch = true; tx = e.x; ty = e.y; }
            }
        nds->SetKeyMask(~keys & 0xFFF);
        if (touch) nds->TouchScreen(tx, ty); else nds->ReleaseScreen();

        for (auto& p : pins) memcpy(&nds->MainRAM[p.first & 0x3FFFFF], &p.second, 4);
        nds->RunFrame();
        // drain audio so the output buffer never fills
        s16 audio[4096 * 2];
        while (nds->SPU.ReadOutput(audio, 4096) > 0) {}

        void *top, *bot;
        nds->GPU.GetFramebuffers(&top, &bot);
        fprintf(hf, "%d %016llx %08x\n", fr, (unsigned long long)Hash((u32*)top, (u32*)bot), nds->GetPC(0));
        if (shots.count(fr) || (shotEvery && fr % shotEvery == 0))
            WritePPM(outdir + "/shot_" + std::to_string(fr) + ".ppm", (u32*)top, (u32*)bot);
        if (dumps.count(fr))
        {
            FILE* d = fopen((outdir + "/ram_" + std::to_string(fr) + ".bin").c_str(), "wb");
            fwrite(nds->MainRAM, 1, 0x400000, d);
            fwrite(nds->ARM9.ITCM, 1, 0x8000, d);
            fwrite(nds->ARM9.DTCM, 1, 0x4000, d);
            fclose(d);
        }
    }
    fclose(hf);
    if (!g_sram.empty())
    {
        FILE* s = fopen((outdir + "/sram.bin").c_str(), "wb");
        fwrite(g_sram.data(), 1, g_sram.size(), s); fclose(s);
    }
    if (g_stopped) { fprintf(stderr, "emulated console stopped (crash?)\n"); return 3; }
    return 0;
}
