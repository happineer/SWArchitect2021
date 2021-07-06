package com.lge.cmuteam3.client.ui;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicLong;

public class UiModel {
    private static final Logger LOG = LoggerFactory.getLogger(UiModel.class);

    AtomicLong count = new AtomicLong(0);
    AtomicLong startTime = new AtomicLong(0);
    AtomicLong previousTime = new AtomicLong(0);
    AtomicLong sum = new AtomicLong(0);
    AtomicLong max = new AtomicLong(0);
    AtomicLong min = new AtomicLong(Long.MAX_VALUE);
    AtomicLong avr = new AtomicLong(0);

    double fullTimeFps = 0;
    double fps = 0;

    ArrayList<Long> histogramData = new ArrayList<>();

    public void updateImageAdded() {
        long currTime = System.currentTimeMillis();
        startTime.compareAndExchange(0, currTime);

        long currCount = count.incrementAndGet();
        long prevTime = previousTime.get();
        previousTime.set(currTime);

        if (count.get() == 1) {
            prevTime = currTime;
        }

        long delay = currTime - prevTime;
        long currSum = sum.addAndGet(delay);
        avr.set(currSum / currCount);

        long prevMax = max.get();
        if (delay > prevMax) {
            max.set(delay);
        }

        long prevMin = min.get();
        if (delay < prevMin && delay != 0) {
            min.set(delay);
        }

        long jitter = delay - 83L;
        if (jitter < 0L) {
            jitter = 0L;
        }
        histogramData.add(jitter);

        long gapTime = currTime - startTime.get() + 1;

        fullTimeFps = (double) count.get() / gapTime * 1000;
        fps = 1000d / delay;

        LOG.info("fullTimeFps:" + fullTimeFps + " fps:" + fps);
    }

    long getCount() {
        return count.get();
    }

    long getMax() {
        return max.get();
    }

    long getMin() {
        return min.get();
    }

    long getAvr() {
        return avr.get();
    }

    public double getFullTimeFps() {
        return fullTimeFps;
    }

    public double getFps() {
        return fps;
    }

    double[] getHistogramData() {
        int size = histogramData.size();
        double[] result = new double[size];
        for (int i = 0; i < size; i++) {
            result[i] = (double) histogramData.get(i);
        }
        return result;
    }
}
