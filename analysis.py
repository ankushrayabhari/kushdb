import numpy as np
import scipy.stats
import csv
import matplotlib.pyplot as plt

data = [[], [], [], []]

stacked = [[], []]

def mean_confidence_interval(data, confidence=0.95):
  a = 1.0 * np.array(data)
  n = len(a)
  m, se = np.mean(a), scipy.stats.sem(a)
  h = se * scipy.stats.t.ppf((1 + confidence) / 2., n-1)
  return m, m-h, m+h

with open('postgres.csv', newline='') as csvfile:
  datareader = csv.reader(csvfile, delimiter=',')
  for row in datareader:
    name=row[0]
    row.pop(0)
    execute = [float(x) for x in row]
    x = np.mean(execute)
    data[0].append(x)
    _,l,h=mean_confidence_interval(execute)
    data[2].append((h-l)/2)

with open('kushdb.csv', newline='') as csvfile:
  datareader = csv.reader(csvfile, delimiter=',')
  for row in datareader:
    name=row[0]
    row.pop(0)
    compile = [float(x) for x in row[::2]]
    execute = [float(x) for x in row[1::2]]

    stacked[0].append(np.mean(compile))
    stacked[1].append(np.mean(execute))

    x = np.mean(execute)
    data[1].append(x)
    _,l,h=mean_confidence_interval(execute)
    data[3].append((h-l)/2)

ind = np.arange(len(data[0]))
width = 0.35       
plt.bar(ind, data[0], width, label='Postgres', yerr=data[2])
plt.bar(ind + width, data[1], width, label='KushDB', yerr=data[3])
plt.ylabel('Runtime (ms)')
plt.title('Runtime by System')
plt.xticks(ind + width / 2, ('Q1', 'Q3', 'Q6', 'Q11', 'Q12'))
plt.legend(loc='best')
plt.savefig('runtime.jpg')

plt.clf()
width = 0.8
plt.bar(ind, stacked[1], width, label='execute', color='gold', bottom=stacked[0])
plt.bar(ind, stacked[0], width, label='compile', color='silver')
plt.xticks(ind, ('Q1', 'Q3', 'Q6', 'Q11', 'Q12'))
plt.ylabel("Runtime (ms)")
plt.legend(loc="upper right")
plt.title("Compile Time vs Execute Time")
plt.savefig('compile.jpg')
