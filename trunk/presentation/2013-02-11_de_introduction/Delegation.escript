var printOut = fn() {
	out("I am a " + this.color + " node.\n");
};

var nodeRed = new ExtObject();
nodeRed.color := "red";
var nodeBlack = new ExtObject();
nodeBlack.color := "black";

var printOutRed = nodeRed -> printOut;
var printOutBlack = nodeBlack -> printOut;

printOutRed(); // Output: I am a red node.
printOutBlack(); // Output: I am a black node.
