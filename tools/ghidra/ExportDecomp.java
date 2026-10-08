// Ghidra headless script: decompile every function (or an address range) and
// write one draft C file per function into the output directory.
//
//   analyzeHeadless <projdir> <name> -import build/arm9_matching.elf \
//       -scriptPath tools/ghidra -postScript ExportDecomp.java <outdir> [lo hi]
//
// The drafts are a starting point for hand decompilation only: they are not
// compiled or committed (they are derived from the game's code).
//@category Decomp

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;

import java.io.File;
import java.io.FileWriter;

public class ExportDecomp extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        File out = new File(args[0]);
        out.mkdirs();
        long lo = args.length > 2 ? Long.decode(args[1]) : 0;
        long hi = args.length > 2 ? Long.decode(args[2]) : 0xFFFFFFFFL;

        DecompInterface ifc = new DecompInterface();
        DecompileOptions opts = new DecompileOptions();
        ifc.setOptions(opts);
        ifc.openProgram(currentProgram);

        int n = 0, failed = 0;
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            long addr = f.getEntryPoint().getOffset();
            if (addr < lo || addr >= hi) continue;
            DecompileResults res = ifc.decompileFunction(f, 120, monitor);
            String name = String.format("%08x.c", addr);
            try (FileWriter w = new FileWriter(new File(out, name))) {
                if (res != null && res.decompileCompleted()) {
                    w.write(res.getDecompiledFunction().getC());
                    n++;
                } else {
                    w.write("/* decompilation failed: " + (res == null ? "?" : res.getErrorMessage()) + " */\n");
                    failed++;
                }
            }
        }
        println("ExportDecomp: " + n + " functions written, " + failed + " failed");
    }
}
