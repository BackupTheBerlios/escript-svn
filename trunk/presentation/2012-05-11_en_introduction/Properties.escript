var Polygon = new Type();
Polygon.vertices @(private, init) := Array;
Polygon.shapeType @(const) := "Polygon";

Polygon.getNumVertices := fn() {
	return this.vertices.count();
};

var polygon = new Polygon();
polygon.getNumVertices();
