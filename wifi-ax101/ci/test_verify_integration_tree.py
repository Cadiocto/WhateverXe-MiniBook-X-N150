#!/usr/bin/env python3
import hashlib, os, stat, subprocess, sys, tempfile
from pathlib import Path
checker=Path(sys.argv[1]).resolve()
def write_manifest(root, entries):
    lines=["type\tmode\tsize\tsha256_or_target\tpath"]
    lines += ["\t".join(map(str,e)) for e in entries]
    p=root/"manifest.tsv"; p.write_text("\n".join(lines)+"\n"); return p
def run(root, manifest):
    return subprocess.run([sys.executable,str(checker),"--source-root",str(root),"--manifest",str(manifest)],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
with tempfile.TemporaryDirectory(prefix="verify-integration-tree-") as td:
    base=Path(td); src=base/"src"; (src/"sub").mkdir(parents=True)
    payload=b"stable-source\n"; (src/"sub"/"file.c").write_bytes(payload)
    (src/".git"/"objects").mkdir(parents=True); (src/".git"/"objects"/"junk").write_text("git metadata")
    os.chmod(src,0o755); os.chmod(src/"sub",0o755)
    entries=[("dir","2755",0,"-","sub"),("file","0644",len(payload),hashlib.sha256(payload).hexdigest(),"sub/file.c")]
    manifest=write_manifest(base,entries); p=run(src,manifest)
    if p.returncode or "INTEGRATION_TREE_EXACT=PASS" not in p.stdout: raise SystemExit("positive test failed:\n"+p.stdout)
    if stat.S_IMODE((src/"sub").stat().st_mode)!=0o2755: raise SystemExit("directory mode not restored")
    print("PASS .git metadata excluded from source inventory")
    print("PASS checkout directory mode restored from exact manifest")
    (src/"sub"/"file.c").write_text("mutated\n"); p=run(src,manifest)
    if p.returncode==0 or "DIFFERENT sub/file.c" not in p.stdout: raise SystemExit("content negative control missed:\n"+p.stdout)
    print("PASS negative control detects source content mutation")
    (src/"sub"/"file.c").write_bytes(payload); os.chmod(src/"sub"/"file.c",0o600); p=run(src,manifest)
    if p.returncode==0 or "DIFFERENT sub/file.c" not in p.stdout: raise SystemExit("mode negative control missed:\n"+p.stdout)
    print("PASS negative control preserves strict file-mode validation")
print("VERIFY_INTEGRATION_TREE_TESTS=PASS 4/4")
