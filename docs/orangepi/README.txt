bootcmd = sunxi_card0_probe;mmcinfo;mmc part;mmc dev 0 0;fatload mmc 0:1 40008000 kernel7.img;go 40008000
mmcinfo;mmc part;mmc dev 0 0;fatload mmc 0:1 40040000 kernel8.img;go 40040000
