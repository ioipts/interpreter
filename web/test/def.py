def test(b):
	if (b < 15):
		x=test(b + 1)
		print(b)
		return x
	else:
		print("ret")
		return b
x = 10
print(test(x))