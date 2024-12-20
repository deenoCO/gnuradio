/*
 * Copyright 2011,2013 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */


/*
 * GNU Radio C++ example creating dial tone
 * ("the simplest thing that could possibly work")
 *
 * Send a tone each to the left and right channels of stereo audio
 * output and let the user's brain sum them.
 *
 * GNU Radio makes extensive use of shared pointers.  Signal processing
 * blocks are typically created by calling a "make" factory function, which
 * returns an instance of the block as a typedef'd shared pointer that can
 * be used in any way a regular pointer can.  Shared pointers created this way
 * keep track of their memory and free it at the right time, so the user
 * doesn't need to worry about it (really).
 *
 */

// Include header files for each block used in flowgraph
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/audio/sink.h>
#include <gnuradio/top_block.h>

int main(int argc, char** argv)
{
    int rate = 48000; // Audio card sample rate
    float ampl = 0.1; // Don't exceed 0.5 or clipping will occur

    // Construct a top block that will contain flowgraph blocks.  Alternatively,
    // one may create a derived class from top_block and hold instantiated blocks
    // as member data for later manipulation.
    gr::top_block_sptr tb = gr::make_top_block("dial_tone");

    // Construct a real-valued signal source for each tone, at given sample rate
    gr::analog::sig_source_f::sptr src0 =
        gr::analog::sig_source_f::make(rate, gr::analog::GR_SIN_WAVE, 350, ampl);
    gr::analog::sig_source_f::sptr src1 =
        gr::analog::sig_source_f::make(rate, gr::analog::GR_SIN_WAVE, 440, ampl);

    // Construct an audio sink to accept audio tones
    gr::audio::sink::sptr sink = gr::audio::sink::make(rate);

    // Connect output #0 of src0 to input #0 of sink (left channel)
    tb->connect(src0, 0, sink, 0);

    // Connect output #0 of src1 to input #1 of sink (right channel)
    tb->connect(src1, 0, sink, 1);

    // Tell GNU Radio runtime to start flowgraph threads; the foreground thread
    // will block until either flowgraph exits (this example doesn't) or the
    // application receives SIGINT (e.g., user hits CTRL-C).
    //
    // Real applications may use tb->start() which returns, allowing the foreground
    // thread to proceed, then later use tb->stop(), followed by tb->wait(), to cleanup
    // GNU Radio before exiting.
    tb->run();

    // Exit normally.
    return 0;
}
