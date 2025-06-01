import sys
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

algorithm = ['gemm', 'prelu'][0]
labeling  = ['annotate', 'legend'][1]

def main():
    measurements = np.genfromtxt('out.csv', delimiter=',', names=True)

    nonZero = np.unique(measurements['nonZero'])

    data_plot = list()
    ys_labels = list()

    for i, z in enumerate(nonZero):
        _data = measurements[np.where(measurements['nonZero'] == z)]
        #_data = np.sort(_data, order=['K', 'N', 'M'])
        _data = np.sort(_data, order=['flops_GEMM'])

        if i == 0:
            input_size = _data[['M', 'K', 'N']]
            data_plot.append(input_size)

            if algorithm == 'gemm':
                # GEMM
                data_plot.append(_data['performance_GEMM'])
                ys_labels.append('GEMM') #   + r'$_\frac{1}{' + f'{z:.0f}' + '}$')

            if algorithm == 'prelu':
                # GEMM_PReLU
                data_plot.append(_data['performance_GEMM_PReLU'])
                ys_labels.append('GEMM$_+$') #   + r'$_\frac{1}{' + f'{z:.0f}' + '}$')

        if algorithm == 'gemm':
            # GEMM
            data_plot.append(_data['performance_sGEMM'])
            ys_labels.append('sGEMM'  + r'$_\frac{1}{' + f'{z:.0f}' + '}$')

        if algorithm == 'prelu':
            # GEMM_PReLU
            data_plot.append(_data['performance_sGEMM_PReLU'])
            ys_labels.append('sGEMM'  + r'$^\frac{1}{' + f'{z:.0f}' + '}_+$')

    x, *ys = data_plot

    plot_performance(x, ys, ys_labels, best=2, peak=1.0)

def plot_performance(x, ys, y_labels, best, peak):
    title = r"Performance of sparseGEMM on Intel Core Ultra 7, 1.4 GHz"
    xlabel = r"input size $(M,K,N)$"
    ylabel = "[flops/cycle]"

    fig, ax = plt.subplots(figsize=(20, 12))

    x_orig = x
    x = np.arange(len(x))

    for i, (y, y_label) in enumerate(zip(ys, y_labels)):
        lw = 3 if i == best else 2
        ms = 7 if i == best else 5
        fw = 'bold' if i == best else 'normal'

        artist, = ax.plot(x, y, marker='o', linewidth=lw, markersize=ms, label=y_label)

        if labeling == 'annotate':
            ax.text(
                x[1], y[1]+0.35, y_label, fontsize=12, transform=ax.transData,
                color=artist.get_color(), weight=fw
            )
    if labeling == 'legend':
        plt.legend()

    # Titles and labels
    bbox = ax.get_yticklabels()[-1].get_window_extent()
    _x_title, _y_title = ax.transAxes.inverted().transform([bbox.x0, bbox.y0])

    ax.set_title(
        title, fontsize=16, weight='bold',
        ha='left',  va='bottom',
        x=_x_title, y=_y_title - 0.04
    )

    bbox = ax.title.get_window_extent()
    _x_ylabel, _y_ylabel = ax.transAxes.inverted().transform([bbox.x0, bbox.y0])
    ax.set_ylabel(
        ylabel, fontsize=14, rotation=0,
        ha='left', va='bottom', x=_x_title, y=_y_title * 0.975 - 0.04,
        labelpad=0.0,
    )
    ax.set_xlabel(xlabel, fontsize=14)

    ax.tick_params(left=False)
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)
    ax.spines['left'].set_visible(False)

    # Grid and background
    ax.set_facecolor('#EDEDED')
    ax.grid(axis='y', color='white')

    # Custom ticks
    xtick_labels = [ f"$({int(m)}, {int(k)}, {int(n)})$" for (m,k,n) in x_orig ]
    plt.xticks(x, xtick_labels, fontsize=12)
    plt.yticks(fontsize=12)

    plt.ylim(0, peak)
    plt.xlim(min(x), max(x))

    # plt.show()
    plt.savefig('performance.png', dpi=300)
if __name__ == '__main__':
    # if len(sys.argv) != 2:
    #     print(f'usage:\n  python3 {sys.argv[0]} path/to/file.csv')
    #     sys.exit(1)

    main()

