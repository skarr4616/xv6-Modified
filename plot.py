import matplotlib.pyplot as plt
import numpy as np
  
# define data values

x1 = [34, 35, 41, 53, 77, 107, 127, 157, 167]
y1 = [0,   1,  2,  3,  4,   3,   4,   3,   4] 

x2 = [38, 40, 47, 61, 89, 119, 139, 169, 179]
y2 = [0,   1,  2,  3,  4,   3,   4,   3,   4] 

x3 = [37, 41, 49, 65, 97, 127, 147, 177, 187]
y3 = [0,   1,  2,  3,  4,   3,   4,   3,   4]  

x4 = [33, 37, 46, 64, 110, 140, 150, 180, 190]
y4 = [0,   1,  2,  3,   4,   3,   4,   3,   4] 

x5 = [40, 45, 57, 75, 105, 109, 125, 155, 165]
y5 = [0,   1,  2,  3,   2,   3,   4,   3,   4] 

plt.step(x1, y1, where='post', label='P1')
plt.step(x2, y2, where='post', label='P2')
plt.step(x3, y3, where='post', label='P3')
plt.step(x4, y4, where='post', label='P4')
plt.step(x5, y5, where='post', label='P5')
plt.legend(loc="lower right")

plt.title("Aging time = 30 ticks")
plt.xlabel("Ticks",fontsize=10)
plt.ylabel("Queue Number",fontsize=10)

plt.show()  # display