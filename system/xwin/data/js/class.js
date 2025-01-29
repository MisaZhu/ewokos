
function f() {
	this.age = 18;
	this.name = "xx";
}

a = new f();
debug(a);

/**/
class Base {
	constructor() {
		this.b = 1;
		debug("Super Base Constructor");
	}

	f() {
		debug("Super Base");
	}
}

class Base1 extends Base {
	constructor() {
		super();
		debug("Super Base1 Constructor");
	}
	f() {
		super.f();
		debug("Super Base1");
	}
}

class Test extends Base1 {
	constructor(cc, bb) {
		super();
		this.c = cc+bb;
	}

	f() {
		super.f();
		debug("this: " + this);
	}
}

a = new Test(2, 4);
a.f();
