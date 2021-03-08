import sympy
import math

# Based on https://foggyhazel.wordpress.com/2018/02/12/catenary-passing-through-2-points/

dx, dy, a = sympy.symbols("dx dy a")

start_x = 100
start_y = 200
end_x = 728.641
end_y = 500.947

vals = [(dx, end_x - start_x), (dy, -end_y - -start_y)]

s = 2.0 * sympy.sqrt(dx ** 2 + dy ** 2)

b = sympy.sqrt(a / dx)
f = 1 / sympy.sqrt(2 * b**2 * sympy.sinh(1 / (2 * b**2)) - 1) - 1 / sympy.sqrt(sympy.sqrt(s**2 - dy**2) / dx - 1)
df = f.diff(a)

print (f"f(x): {f}")
print (f"df(x): {df}")

def solve(f):
    return (f.subs(vals).subs(a, x)).evalf()

x = 513702
print (solve(b))
print (solve(1 / (2 * (b**2))))
print (solve(2 * (b**2) * sympy.sinh(1 / (2 * (b**2)))))
print (solve(1 / sympy.sqrt(2 * (b**2) * sympy.sinh(1 / (2 * (b**2))) - 1)))
exit()

x = 10
for i in range(10):
    print (f"Iteration {i}, x={x} f(x)={solve(f)} df(x)={solve(df)}")
    x = x - solve(f / df)
    if (abs(solve(f)) < 1E-5):
        break
else:
    raise Exception("Failed to converge")

alpha = x
x_offset = solve(0.5 * (dx + a * sympy.log((s + dy) / (s - dy))))
y_offset = solve(a * sympy.cosh((0 - x_offset) / a))
print (f"Final alpha={alpha}, x_offset={x_offset}, y_offset={y_offset}")

def solve(x):
    return -alpha * math.cosh((x - x_offset) / alpha) + y_offset

x = 0
print (f"Final eval at x={x}: {solve(x)}")
x = 200
print (f"Final eval at x={x}: {solve(x)}")
x = 400
print (f"Final eval at x={x}: {solve(x)}")
