function f( ) {
	let x = "closure test: ";
	let y = 0;

	function f2() {
		function f1() {
			debug(x + y);
			y++;
		}
		return f1;
	}
	return f2();
}

fc = f();
while(true) {
	fc();
}
