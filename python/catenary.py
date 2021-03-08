import sympy
import math

# Based on https://foggyhazel.wordpress.com/2018/02/12/catenary-passing-through-2-points/

x0, x1, y0, y1, a = sympy.symbols("x0 x1 y0 y1 a")

vals = [(x0, 100), (x1, 500), (y0, 200), (y1, 500)]

h = x1 - x0
v = y0 - y1
s = 1.5 * sympy.sqrt(h ** 2 + v ** 2)

f = 2 * a * sympy.sinh(h / (2 * a)) - sympy.sqrt(s ** 2 - v ** 2)
df = f.diff(a)

def solve(f):
    return (f.subs(vals).subs(a, x)).evalf()

x = 50
for i in range(10):
    print (f"Iteration {i}, x={x} f(x)={solve(f)} df(x)={solve(df)}")
    x = x - solve(f / df)
    if (solve(f) < 1E-3):
        break
else:
    raise Exception("Failed to converge")

alpha = x
x_offset = solve(x0 + 0.5 * (h + a * sympy.log((s + v) / (s - v))))
y_offset = solve(y0 - a * sympy.cosh((x0 - x_offset) / a))
print (f"Final alpha={alpha}, x_offset={x_offset}, y_offset={y_offset}")

def solve(x):
    return alpha * math.cosh((x - x_offset) / alpha) + y_offset

x = 100
print (f"Final eval at x={x}: {solve(x)}")
x = 500
print (f"Final eval at x={x}: {solve(x)}")
