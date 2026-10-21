const fs = require("node:fs");
const path = require("node:path");
const crypto = require("node:crypto");
const { spawnSync } = require("node:child_process");
const runtimeRoot = process.env.CA_TOOL_NODE_RUNTIME_ROOT ||
    path.resolve(path.dirname(process.execPath), "..");
const support = require(path.join(runtimeRoot, "lib/codex/native-support"));
const artifact = path.resolve(process.argv[2]);
const runner = path.resolve(process.argv[3]);
const marker = process.env.ID_ARTIFACT_MARKER ||
    "/var/mobile/Library/Logs/IslandDigest-0.1.4.startup-pending";
if (fs.existsSync(marker)) {
    throw new Error("Existing startup marker: stop without modifying production startup state");
}
const digest = file => crypto.createHash("sha256").update(fs.readFileSync(file)).digest("hex");
const originalHash = digest(artifact);
let loadTarget = artifact;
if (process.argv[4] === "--resign-copy") {
    loadTarget = path.join(path.dirname(artifact), "ResignedPackagedIslandDigest.node");
    if (loadTarget === artifact) throw new Error("Signing requires a separate copy");
    fs.copyFileSync(artifact, loadTarget);
    support.signAddons([loadTarget]);
    process.stdout.write("TEST COPY RESIGNED: this does not validate the package signature\n");
}
support.signAddons([runner]);
const staged = support.stageAddonForLoad(loadTarget);
if (digest(staged) !== digest(loadTarget)) throw new Error("Artifact bytes changed during staging");
if (digest(artifact) !== originalHash) throw new Error("Original artifact was modified");
process.stdout.write(`Artifact SHA-256 ${originalHash}\n`);
const result = spawnSync(process.execPath, ["-e", `require(${JSON.stringify(runner)})`], {
    env: { ...process.env, ID_ARTIFACT_PATH: staged, ID_ARTIFACT_MARKER: marker },
    encoding: "utf8",
    timeout: 45000
});
process.stdout.write(result.stdout || "");
process.stderr.write(result.stderr || "");
if (result.error) process.stderr.write(`${result.error.message}\n`);
process.exit(result.status === 0 ? 0 : 1);
