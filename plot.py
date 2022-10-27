import matplotlib.pyplot as plt
import numpy as np
import matplotlib.patches as mpatches
from colour import Color
# 输入：finish_time，每个工件加工的结束时间，格式与程序输出的每个用例格式。每行第一个为加工的工件序号，其余依次为在每道工序上加工的结束时间。
finish_time = np.genfromtxt("result2.txt", dtype=int)
# 输入：time，工件加工时间，格式与每个用例相同。
time = np.genfromtxt("time2.txt", dtype=int)
# 局部变量：continue_time，处理后的工件加工时间，continue_time[i][j]为第i+1个工件在第j+1台机器上的加工时间。
continue_time = []
# 局部变量：start_time，工件加工的开始时间，start_time[i][j]为第i+1个工件在第j+1台机器上开始加工的时间。
start_time = []
# 局部变量：label，工件的加工顺序，label[i]为第i+1个加工的工件
label = []
# 对输入进行处理，得到上述局部变量。
for i in range(time.shape[0]):
    continue_time.append([])
    for j in range(time.shape[1]):
        if j % 2 == 1:
            continue_time[i].append(time[i][j])
for i in range(finish_time.shape[0]):
    label.append(finish_time[i][0])
for i in range(finish_time.shape[0]):
    start_time.append([])
    for j in range(finish_time.shape[1]):
        if j != 0:
            start_time[i].append(finish_time[i][j] - continue_time[label[i]][j - 1])
# 画图时正确显示中文
plt.rcParams['font.sans-serif'] = ['SimHei']
# 设置甘特图的颜色
red = Color("red")
colors = list(red.range_to(Color("purple"), len(start_time)))
colors = [color.rgb for color in colors]
# 利用barh画出水平柱状图
m = range(finish_time.shape[0])
n = range(finish_time.shape[1] - 1)
for i in m:
    for j in n:
        plt.barh(y=n[j] + 1, width=continue_time[label[i]][j], left=start_time[i][j], color=colors[i])
# 设置图例文字
labels = [''] * len(continue_time)
for f in m:
    labels[f] = "工件%d" % (label[f] + 1)
# 添加图例
patches = [mpatches.Patch(color=colors[i], label="{:s}".format(labels[i])) for i in range(len(continue_time))]
plt.legend(handles=patches, loc=4)
# 添加XY轴标签
plt.xlabel("加工时间/分钟", fontsize=15)
plt.ylabel("加工机器", fontsize=15)
plt.show()
