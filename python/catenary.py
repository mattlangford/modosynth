import sympy
import math

# Based on https://foggyhazel.wordpress.com/2018/02/12/catenary-passing-through-2-points/

x0, x1, y0, y1, a = sympy.symbols("x0 x1 y0 y1 a")

start_x = 100
start_y = 200
end_x = 500
end_y = 500

vals = [(x0, 0), (x1, end_x - start_x), (y0, 0), (y1, end_y - start_y)]
print (end_x - start_x, end_y - start_y)

h = x1 - x0
v = y1 - y0
s = 2.0 * sympy.sqrt(h ** 2 + v ** 2)

f = 2 * a * sympy.sinh(h / (2 * a)) - sympy.sqrt(s ** 2 - v ** 2)
df = f.diff(a)

def solve(f):
    return (f.subs(vals).subs(a, x)).evalf()

x = 50
for i in range(10):
    print (f"Iteration {i}, x={x} f(x)={solve(f)} df(x)={solve(df)}")
    x = x - solve(f / df)
    if (solve(f) < 1E-5):
        break
else:
    raise Exception("Failed to converge")

alpha = x
x_offset = solve(x0 + 0.5 * (h + a * sympy.log((s - v) / (s + v))))
y_offset = solve(y0 - a * sympy.cosh((x0 - x_offset) / a))
print (f"Final alpha={alpha}, x_offset={x_offset}, y_offset={y_offset}")

def solve(x):
    return alpha * math.cosh((x - x_offset) / alpha) + y_offset

x = 0
print (f"Final eval at x={x}: {solve(x)}")
x = 200
print (f"Final eval at x={x}: {solve(x)}")
x = 400
print (f"Final eval at x={x}: {solve(x)}")
