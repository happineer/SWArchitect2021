package com.lge.cmuteam3.client.ui;

import java.util.concurrent.atomic.AtomicLong;

public class UiModel {
    AtomicLong count = new AtomicLong(0);
    AtomicLong previousTime = new AtomicLong(0);
    AtomicLong sum = new AtomicLong(0);
    AtomicLong max = new AtomicLong(0);
    AtomicLong min = new AtomicLong(Long.MAX_VALUE);
    AtomicLong avr = new AtomicLong(0);

    public void updateImageAdded() {
        long currTime = System.currentTimeMillis();
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
}
