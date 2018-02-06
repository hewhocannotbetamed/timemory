#!/usr/bin/env python
#
# MIT License
#
# Copyright (c) 2018, The Regents of the University of California,
# through Lawrence Berkeley National Laboratory (subject to receipt of any
# required approvals from the U.S. Dept. of Energy).  All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

## @file array_test.py
## A test using weakrefs and classes for the timemory module
##

import sys
import os
import numpy as np
import time
import gc
import timemory

import weakref


# ---------------------------------------------------------------------------- #
class auto_disk_array_weakref(object):
    """
    Base class enabling the use a __finalize__ method without all the problems
    associated with __del__ and reference cycles.

    If you call enable_finalizer(), it will call __finalize__ with a single
    "ghost instance" argument after the object has been deleted. Creation
    of this "ghost instance" does not involve calling the __init__ method,
    but merely copying the attributes whose names were given as arguments
    to enable_finalizer().

    A Finalizable can be part of any reference cycle, but you must be careful
    that the attributes given to enable_finalizer() don't reference back to
    self, otherwise self will be immortal.
    """

    __wr_map = {}
    __wr_id = None
    finished = False
    ghost_attributes = ('resource', )

    @timemory.util.auto_timer(is_class=True)
    def bind_finalizer(self, *attrs):
        """
        Enable __finalize__ on the current instance.
        The __finalize__ method will be called with a "ghost instance" as
        single argument.
        This ghost instance is constructed from the attributes whose names
        are given to bind_finalizer(), *at the time bind_finalizer() is called*.
        """
        cls = type(self)
        ghost = object.__new__(cls)
        ghost.__dict__.update((k, getattr(self, k)) for k in attrs)
        cls_wr_map = cls.__wr_map
        def _finalize(ref):
            try:
                ghost.__finalize__()
            finally:
                del cls_wr_map[id_ref]
        ref = weakref.ref(self, _finalize)
        id_ref = id(ref)
        cls_wr_map[id_ref] = ref
        self.__wr_id = id_ref

    @timemory.util.auto_timer(is_class=True)
    def remove_finalizer(self):
        """
        Disable __finalize__, provided it has been enabled.
        """
        if self.__wr_id:
            cls = type(self)
            cls_wr_map = cls.__wr_map
            del cls_wr_map[self.__wr_id]
            del self.__wr_id

    @timemory.util.auto_timer(is_class=True)
    def enable_auto_rollback(self):
        self.bind_finalizer(*self.ghost_attributes)

    #@timemory.util.auto_timer(is_class=True)
    #def commit(self):
        #assert not self.finished
        #self.remove_finalizer()
        #self.do_commit()
        #self.finished = True

    #@timemory.util.auto_timer(is_class=True)
    #def rollback(self):
        #assert not self.finished
        #self.remove_finalizer()
        #self.do_rollback(auto=False)
        #self.finished = True

    @timemory.util.auto_timer(is_class=True)
    def __finalize__(ghost):
        print('calling __finalize__')
        ghost.do_rollback(auto=True)

    @timemory.util.auto_timer(is_class=True)
    def __init__(self, resource):
        self.resource = resource
        self.enable_auto_rollback()

    @timemory.util.auto_timer(is_class=True)
    def do_commit(self):
        pass

    @timemory.util.auto_timer(is_class=True)
    def do_rollback(self, auto):
        pass


# ---------------------------------------------------------------------------- #
class auto_disk_array(np.ndarray):
    """
    A special wrapper class around an np.ndarray that handles synchronization
    between FsBuffer storage and memory
    """
    @timemory.util.auto_timer()
    def __new__(cls, input_array, _name):
        # Input array is an already formed ndarray instance
        # We first cast to be our class type
        obj = np.asarray(input_array).view(cls)
        # add the new attribute to the created instance
        obj._name = _name
        # Finally, we must return the newly created object:
        return obj


    @timemory.util.auto_timer(is_class=True)
    def __array_finalize__(self, obj):
        # see InfoArray.__array_finalize__ for comments
        if obj is None: return
        self._name = getattr(obj, '_name', None)


    @timemory.util.auto_timer(is_class=True)
    def __del__(self):
        print ('--> deleting {}({})...'.format(type(self).__name__, self._name))

    @timemory.util.auto_timer(is_class=True)
    def __call__(self):
        print ('--> calling {}({})...'.format(type(self).__name__, self._name))
        return self


# ---------------------------------------------------------------------------- #
@timemory.util.auto_timer(report_at_exit=True)
def run(weak_ref):

    @timemory.util.auto_timer()
    def create(weak_ref):

        if weak_ref is not None:
            #print ("type: {}".format(type(weak_refs[0].resource()).__name__))
            return weak_ref.resource()

        arr = np.ones([3000,3000], dtype=np.float128)
        ret = auto_disk_array(arr, 'test')
        #print ('\n{} is gc: {}, {} is gc: {}'.format(type(arr).__name__,
        #    gc.is_tracked(arr), type(ret).__name__, gc.is_tracked(ret)))

        #print ('ret: {}'.format(ret))
        #ret.tofile('test_array.txt')
        #f = open('test_array.txt', 'w')
        #f.write('{}'.format(ret))
        #f.close()
        weak_ref = auto_disk_array_weakref(ret)
        return weak_ref

    weak_ref = create(weak_ref)
    ref = weak_ref.resource()
    if ref is not None:
        ref += np.ones([3000,3000], dtype=np.float128)

    if weak_ref is not None:
        print ("--> weak_ref type: {}".format(type(weak_ref).__name__))
        print ("--> weak_ref resource type: {}".format(type(weak_ref.resource()).__name__))

    return ref

# ---------------------------------------------------------------------------- #
@timemory.util.auto_timer(report_at_exit=True)
def main():

    print ("\nmain start")
    weak_ref = None
    for i in range(5):
        ref = run(weak_ref)
        print ('Weak ref: {}'.format(type(weak_ref).__name__))

    print ("\nmain end \n")


# ---------------------------------------------------------------------------- #
@timemory.util.auto_timer(report_at_exit=True)
def dummy():

    @timemory.util.auto_timer()
    def do_sleep():
        time.sleep(1)

    do_sleep()


# ---------------------------------------------------------------------------- #
def measure(name, _time = 1, _rss = None):
    t = timemory.timer(name)
    t.start()
    time.sleep(_time)
    t.stop()
    if _rss is not None:
        t -= _rss
    print('\n{}\n'.format(t))


# ---------------------------------------------------------------------------- #
def run_test():
    timemory.enable_signal_detection()

    rss = timemory.rss_usage()
    rss.record()
    print('\nRSS at initialization: {}\n'.format(rss))

    measure('begin', _rss = rss)

    main()
    main()
    main()
    dummy()

    timing_manager = timemory.timing_manager()
    timing_manager -= rss
    print('\nTiming report:\n{}'.format(timing_manager))
    #timing_manager.report(no_min = True)

    measure('end', _rss = rss)

    timemory.disable_signal_detection()

# ---------------------------------------------------------------------------- #
if __name__ == '__main__':
    run_test()