very simple read only RAM file system for kernel.

	disk: [file0][file1][file...]

	file: [name_len: 4][name: name_len][content_len][content]

.Make a ramdisk image file, put source dir's files into 

	mkramdisk [ramdisk image file name] [sourc dir name]
