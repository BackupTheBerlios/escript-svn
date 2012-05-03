var Player = new Type();
Player.x @(private) := 0;
Player.y @(private) := 0;
Player.move := fn(Number dx, Number dy) {
	this.x += dx;
	this.y += dy;
};
Player.printPos := fn() {
	outln("Position: (", this.x, ", ", this.y, ")");
};

var playerA = new Player();
playerA.move(5, 7);
playerA.printPos(); // Output: Position: (5, 7)
