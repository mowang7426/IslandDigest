const path = require("node:path");
const { spawnSync } = require("node:child_process");
const runtimeRoot = process.env.CA_TOOL_NODE_RUNTIME_ROOT ||
    path.resolve(path.dirname(process.execPath), "..");
const support = require(path.join(runtimeRoot, "lib/codex/native-support"));
const addon = path.resolve(process.argv[2]);
support.signAddons([addon]);
const result = spawnSync(process.execPath, ["-e", `require(${JSON.stringify(addon)})`], {
    encoding: "utf8",
    timeout: 15000
});
process.stdout.write(result.stdout || "");
process.stderr.write(result.stderr || "");
if (result.error) process.stderr.write(`${result.error.message}\n`);
process.exit(result.status === 0 ? 0 : 1);
