#!/usr/bin/env python
# coding: utf-8

#import os
import time
from threading import Thread


class task_thread_class(Thread):

    def __init__(self, idx, a, b):
        self.count = None
        self.state = 'idle'
        self.idx = idx
        self.a = a
        self.b = b
        super().__init__()

    def run(self):
        for i in range(self.a, self.b):
            print(self.ident, self.idx, i)
            self.count = i
            time.sleep(1)
        self.state = 'idle'


def task_thread_fun(a, b):
    for i in range(a, b):
        print(threading.current_thread().ident, i)


def upd_stats(threads):
    state = []
    for t in threads:
        #print(t.ident, t.state, t.count)
        state.append({ f'state{t.idx}': t.state, 'count': t.count })

    print(state)


def main():
    ranges = [(1,10), (2,8), (3,7)]
    threads = []
    idx = 0
    for a, b in ranges:
        #t = threading.Thread(target=task_thread_fun, args=(a, b))
        t = task_thread_class(idx, a, b)
        threads.append(t)
        idx += 1

    for t in threads:
        t.state = 'running'
        t.start()

    upd_stats(threads)

    time.sleep(5)

    upd_stats(threads)

    time.sleep(5)

    upd_stats(threads)

    for t in threads:
        t.join()


if __name__ == "__main__":
    main()
