mu = 2e-5 ## lb s / ft2
p1 = 5 ##psi
p2 = 1 ##psi

D = 2 ## in
r = D/2

L = 10 ##ft

##Q = 3.14 * (p1-p2) * r**4 / (8 * mu * L)


"""
lb * in4 * ft2
______
in2 * ft * lb * s


lb * in2 * ft
_________
lb * s 

12 * in3
_____

s

"""

Q = 3.14 * (p1-p2) * r**4 / (8 * mu * L) * 12 ##in3/s /231 * 60
