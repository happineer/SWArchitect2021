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

    double averageFps = 0;
    int fps = 0;
    double averageJitter = 0;

    ArrayList<Long> histogramData = new ArrayList<>();
    ArrayList<Long> latencyData = new ArrayList<>();

    public void updateImageAdded(long latency) {
        long updateTime = System.currentTimeMillis();

        // Save the first time
        startTime.compareAndSet(0, updateTime);

        // frame count
        long currCount = count.incrementAndGet();

        long prevTime = previousTime.get();
        previousTime.set(updateTime);

        if (count.get() == 1) {
            prevTime = updateTime;
        }

        latencyData.add(updateTime);

        long currSum = sum.addAndGet(latency);
        avr.set(currSum / currCount);

        long prevMax = max.get();
        if (latency > prevMax) {
            max.set(latency);
        }

        long prevMin = min.get();
        if (latency < prevMin && latency != 0) {
            min.set(latency);
        }

        long jitter = Math.abs(updateTime - prevTime);
        histogramData.add(jitter);

        long duration = updateTime - startTime.get() + 1;
        averageFps = (double) count.get() / duration * 1000;

        int count = 0;
        for (int i = latencyData.size() - 1; i >= 0 ; i--) {
            if ((updateTime - latencyData.get(i)) <= 1000) {
                count++;
            } else {
                break;
            }
        }
        fps = count;
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

    public double getAverageFps() {
        return averageFps;
    }

    public int getFps() {
        return fps;
    }

    double[] getHistogramData() {
        int size = histogramData.size();
        double[] result = new double[size];
        double sum = 0;
        for (int i = 0; i < size; i++) {
            result[i] = (double) histogramData.get(i);
            sum += result[i];
        }
        averageJitter = sum / size;

        return result;
    }

    public long getElapsedTime() {
        return System.currentTimeMillis() - startTime.get();
    }

    public double getAverageJitter() {
        return averageJitter;
    }
}
