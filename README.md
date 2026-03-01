# LINFO2347 - fuzzer

To create an archive corresponding to this format you can use: 
```command
tar --posix --pax-option delete=".*" --pax-option delete="*time*" --no-xattrs --no-acl --no-selinux -c fichier1 fichier2 ... > archive.tar
```