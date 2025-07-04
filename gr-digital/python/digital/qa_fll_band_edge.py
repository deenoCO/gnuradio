#!/usr/bin/env python
#
# Copyright 2011-2013 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
#


import random
import math
import numpy

from gnuradio import gr, gr_unittest, digital, filter, blocks, analog


class test_fll_band_edge_cc(gr_unittest.TestCase):

    def setUp(self):
        random.seed(0)
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test01(self):
        sps = 4
        rolloff = 0.35
        bw = 2.3e-3
        ntaps = 45

        # Create pulse shape filter
        rrc_taps = filter.firdes.root_raised_cosine(
            sps, sps, 1.0, rolloff, ntaps)

        # The frequency offset to correct
        foffset = 0.2 / (2.0 * math.pi)

        # Create a set of 1's and -1's, pulse shape and interpolate to sps
        random.seed(42)  # Fix seed
        data = [2.0 * random.randint(0, 2) - 1.0 for i in range(200)]
        self.src = blocks.vector_source_c(data, False)
        self.rrc = filter.interp_fir_filter_ccf(sps, rrc_taps)

        # Mix symbols with a complex sinusoid to spin them
        self.nco = analog.sig_source_c(1, analog.GR_SIN_WAVE, foffset, 1)
        self.mix = blocks.multiply_cc()

        # FLL will despin the symbols to an arbitrary phase
        self.fll = digital.fll_band_edge_cc(sps, rolloff, ntaps, bw)

        # Create sinks for all outputs of the FLL
        # we will only care about the freq and error outputs
        self.vsnk_frq = blocks.vector_sink_f()
        self.nsnk_fll = blocks.null_sink(gr.sizeof_gr_complex)
        self.nsnk_phs = blocks.null_sink(gr.sizeof_float)
        self.nsnk_err = blocks.null_sink(gr.sizeof_float)

        # Connect the blocks
        self.tb.connect(self.nco, (self.mix, 1))
        self.tb.connect(self.src, self.rrc, (self.mix, 0))
        self.tb.connect(self.mix, self.fll, self.nsnk_fll)
        self.tb.connect((self.fll, 1), self.vsnk_frq)
        self.tb.connect((self.fll, 2), self.nsnk_phs)
        self.tb.connect((self.fll, 3), self.nsnk_err)
        self.tb.run()

        N = 700
        dst_data = self.vsnk_frq.data()[N:]

        expected_result = len(dst_data) * [-0.20, ]
        self.assertFloatTuplesAlmostEqual(expected_result, dst_data, 4)

    def test02_noisy(self):
        sps = 4
        rolloff = 0.35
        bw = 2.3e-3
        ntaps = 45
        noise_std = 1 / 16
        length = 2 * 10**5

        # Create pulse shape filter
        rrc_taps = filter.firdes.root_raised_cosine(
            sps, sps, 1.0, rolloff, ntaps)

        # The frequency offset to correct
        foffset = 0.2 / (2.0 * math.pi)

        # Create a set of 1's and -1's, pulse shape and interpolate to sps
        random.seed(42)  # Fix seed
        data = [2.0 * random.randint(0, 2) - 1.0 for i in range(length)]
        noise = numpy.random.standard_normal(size=len(data)) + 1j * numpy.random.standard_normal(size=len(data))
        noise *= noise_std
        self.src = blocks.vector_source_c(numpy.array(data) + noise, False)
        self.rrc = filter.interp_fir_filter_ccf(sps, rrc_taps)

        # Mix symbols with a complex sinusoid to spin them
        self.nco = analog.sig_source_c(1, analog.GR_SIN_WAVE, foffset, 1)
        self.mix = blocks.multiply_cc()

        # FLL will despin the symbols to an arbitrary phase
        self.fll = digital.fll_band_edge_cc(sps, rolloff, ntaps, bw)

        # Create sinks for all outputs of the FLL
        # we will only care about the freq and error outputs
        self.vsnk_frq = blocks.vector_sink_f()
        self.nsnk_fll = blocks.null_sink(gr.sizeof_gr_complex)
        self.nsnk_phs = blocks.null_sink(gr.sizeof_float)
        self.nsnk_err = blocks.null_sink(gr.sizeof_float)

        # Connect the blocks
        self.tb.connect(self.nco, (self.mix, 1))
        self.tb.connect(self.src, self.rrc, (self.mix, 0))
        self.tb.connect(self.mix, self.fll, self.nsnk_fll)
        self.tb.connect((self.fll, 1), self.vsnk_frq)
        self.tb.connect((self.fll, 2), self.nsnk_phs)
        self.tb.connect((self.fll, 3), self.nsnk_err)
        self.tb.run()

        N = int(noise_std * 10000)
        self.assertLess(N, len(self.vsnk_frq.data()))
        dst_data = self.vsnk_frq.data()[N:]

        expected_result = len(dst_data) * [-0.20, ]
        self.assertFloatTuplesAlmostEqual(expected_result, dst_data, math.ceil(-math.log10(noise_std)))


if __name__ == '__main__':
    gr_unittest.run(test_fll_band_edge_cc)
