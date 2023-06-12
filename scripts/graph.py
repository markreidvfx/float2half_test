import os
import csv
import argparse
from pprint import pprint

import matplotlib.pyplot as plt
from matplotlib.ticker import EngFormatter

import numpy as np


def extract_graph_data(path):
    graphs  = []

    with open(path, newline='') as csvfile:
        csv_data = list(csv.reader(csvfile))

        info = csv_data[0]
        os_info = {}
        os_info['cpu_name'] = None
        os_info['arch'] = None
        os_info['extensions'] = None
        if len(info) >= 1:
            os_info['arch'] = info[0]

        if len(info) >= 2:
            os_info['cpu_name']  = info[1]
        if len(info) >= 3:
            os_info['extensions'] = info[2]

        info = csv_data[1]

        os_info['os_name'] = info[0]
        os_info['compiler'] = info[1]

        item = {}
        index = 3
        while index < len(csv_data):
            row = csv_data[index]

            if row and not item:
                item['type'] = row[0].strip()
                item['name'] = row[1].strip()
                item['os_info'] = os_info
                item['headers'] = csv_data[index+1]
                item['data'] = []
                index += 1

            elif not row:
                if item:
                    graphs.append(item)
                    item = {}
            else:
                item['data'].append(row)

            index += 1

    return graphs

def draw_perf_graph(graph_a, graph_b):
    labels = []
    values_a = []
    values_b = []

    error_min_a = []
    error_min_b = []

    error_max_a = []
    error_max_b = []

    for row_a, row_b in zip(graph_a['data'], graph_b['data']):
        labels.append(row_a[0])

        va = float(row_a[2])
        vb = float(row_b[2])

        values_a.append(va)
        values_b.append(vb)

        error_min_a.append(abs(float(row_a[1]) - va))
        error_min_b.append(abs(float(row_b[1]) - vb))

        error_max_a.append(abs(float(row_a[3]) - va))
        error_max_b.append(abs(float(row_b[3]) - vb))

    y_pos = np.arange(len(labels))
    width = 0.4

    fig, ax = plt.subplots()
    fig.set_figwidth(10)

    ax.barh(y_pos-0.2, values_a, width, xerr=[error_min_a, error_max_a], align='center')
    ax.barh(y_pos+0.2, values_b, width, xerr=[error_min_b, error_max_b], align='center')

    ax.set_yticks(y_pos, labels)

    plt.legend([graph_a['name'],
                graph_b['name']], loc='upper right')

    info = graph_a['os_info']

    out_filename = f"{info['cpu_name']}\n{info['os_name']} {info['compiler']}"
    out_filename = out_filename.strip()
    title = f"1920x1080 RGBA frame convert speed\n{out_filename}"

    ax.set_title(title)
    plt.subplots_adjust(top=0.85, left=0.2)
    ax.set_xlabel('Seconds (less is better)')

    # plt.show()
    out_filename = out_filename.replace("\n", " ")
    filename = "".join([c for c in out_filename if c.isalpha() or c.isdigit() or c==' ']).rstrip()
    filename = filename.replace(" ", "_")
    outdir = "images"
    outimage = os.path.join(outdir, f"{filename}.png")
    if not os.path.exists(outdir):
        os.makedirs(outdir)
    plt.savefig(outimage)

def draw_accuracy_graph(graph_data):
    group_data = []
    labels = []
    error_min = []
    error_max = []
    graph_type = graph_data['type']

    total_values = None

    for row in graph_data['data']:
        labels.append(row[0])
        if graph_type == 'perf_test':
            v = float(row[2])
            group_data.append(v)
            err_mn = abs(float(row[1]) - v)
            err_mx = abs(float(row[3]) - v)

            error_min.append(err_mn)
            error_max.append(err_mx)
        else:
            v = float(row[1])
            r = float(row[2])
            total_values = int(r)
            group_data.append((r-v)/r * 100.0)


    info = graph_data['os_info']
    plt.rcdefaults()
    fig, ax = plt.subplots()
    y_pos = list(range(len(labels)))

    fig.set_figwidth(10)

    name = graph_data['name']

    colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd',
              '#8c564b', '#e377c2', '#7f7f7f', '#bcbd22', '#17becf']

    if graph_type == 'perf_test':
        title = f"{name}\n{info['cpu_name']}\n{info['os_name']} {info['compiler']}"
        hbars = ax.barh(y_pos, group_data, xerr=[error_min, error_max], align='center', color=colors)
        # ax.bar_label(hbars, fmt='%.2f', label_type='center',  padding=30)
        ax.set_xlabel('Seconds')
    else:
        title = name
        hbars = ax.barh(y_pos, group_data, color=colors)
        ax.set_xlabel(f'Percentage Of Values That Match ( Out Of {total_values} Values)')
        ax.bar_label(hbars, fmt='%.2f%%', label_type='center', padding=30)
        ax.xaxis.set_major_formatter(EngFormatter(unit="%"))

    ax.set_yticks(y_pos, labels)

    # print(info)
    ax.set_title(title)
    plt.subplots_adjust(top=0.85, left=0.2)
    # plt.show()

    filename = "".join([c for c in title if c.isalpha() or c.isdigit() or c==' ']).rstrip()
    filename = filename.replace(" ", "_")
    outdir = "images"
    outimage = os.path.join(outdir, f"{filename}.png")
    if not os.path.exists(outdir):
        os.makedirs(outdir)
    plt.savefig(outimage)

def run_cli():
    parser = argparse.ArgumentParser(description="Create graphs from test results")
    parser.add_argument("csv_file", type=str)
    parser.add_argument("-a", "--accuracy-graph", action='store_true', default=False)

    args = parser.parse_args()

    graphs = extract_graph_data(args.csv_file)

    draw_perf_graph(graphs[0], graphs[1])

    if args.accuracy_graph:
        for g in graphs[2:]:
            draw_accuracy_graph(g)


if __name__ == "__main__":

    run_cli()
# plt.rcdefaults()
# fig, ax = plt.subplots()