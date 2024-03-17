let str = "MR:你好世界!!!";
console.log(str + ":raw length = " + str.length());

let u = new UTF8(str);
console.log(u.toString() + ":utf8 length = " + u.length());

u.set(4, "棒");
u.set(5, "");
console.log(u.toString());

/*let ur = new UTF8Reader(str);
console.log("utf8reader: ");
while(true) {
	let w = ur.read();
	if(w.length() == 0) {
		console.log("\n");
		break;
	}
	console.log("[" + w + "]");
}
*/
u = u.substr(3, 5);
console.log(u.toString());
