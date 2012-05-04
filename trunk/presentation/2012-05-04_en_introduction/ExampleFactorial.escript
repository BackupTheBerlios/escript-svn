var factorialRecursive = fn(Number n) {
	return (n == 0) ? 1 : thisFn(n - 1) * n;
};
var factorialIterative = fn(Number n) {
	var product = 1;
	for(var i = 2; i <= n; ++i) {
		product *= i;
	}
	return product;
};

outln(factorialRecursive(6)); // Output: 720
outln(factorialIterative(7)); // Output: 5040
