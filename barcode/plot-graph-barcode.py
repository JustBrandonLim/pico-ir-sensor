import matplotlib.pyplot as graph
import numpy as np
import os

raw_x = []
raw_y = []
avg_x = []
avg_y = []
avg_rng_x = []
avg_rng_y = []

path = "C:/Users/brand/Desktop"

def parse(raw_x, raw_y, avg_x, avg_y, avg_rng_x, avg_rng_y, lines):
    for i, line in enumerate(lines):
        #if i <= 1000:
        #    continue

        if i >= 20000 and i <= 40000:
            try:
                data = line.split(":")

                raw = int(data[0])
                avg = int(data[1])
                avg_rng = int(data[2])

                if raw >= 4096 or avg >= 4096 or avg_rng >= 4096:
                    print(i)

                raw_x.append(i)
                raw_y.append(raw)

                avg_x.append(i)
                avg_y.append(avg)

                avg_rng_x.append(i)
                avg_rng_y.append(avg_rng)
            except:
                continue

file = open(os.path.join(path, "teraterm.log"))
file_lines = file.readlines()

parse(raw_x, raw_y, avg_x, avg_y, avg_rng_x, avg_rng_y, file_lines)

graph.plot(np.array(raw_x), np.array(raw_y), color="red", label="Raw", linewidth=1)#, linestyle="dashed")
graph.plot(np.array(avg_x), np.array(avg_y), color="green", label="Average (w. 50 samples)", linewidth=1)#, linestyle="dotted")
graph.plot(np.array(avg_rng_x), np.array(avg_rng_y), color="blue", label="Average (w. 50 samples) and Range (± 10)", linewidth=1)#, linestyle="dashed")

graph.title("IR Sensor")
graph.xlabel("Count")
graph.ylabel("ADC Reading")
graph.legend()

graph.savefig(os.path.join(path, "graph.png"))