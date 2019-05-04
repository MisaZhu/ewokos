#!/bin/awk -f

/^=====/ {
	print key;
	if (is_revision) is_revision = 0;
}
#!/^=====/ { if (is_revision) print $0; }
/^===== Revision '$key' =====/ {
	print key;
	print $0;
	is_revision = 1;
}
BEGIN {
	key=ENVIRON["REVISION"];
	is_revision = 0;
}
