package com.lge.cmuteam3.client.network;

import java.util.concurrent.ArrayBlockingQueue;

public class NetworkUiLogManager {
    private static NetworkUiLogManager instance;

    private final ArrayBlockingQueue<Log> queue = new ArrayBlockingQueue<>(50);
    private OnLogAddedListener listener;

    private NetworkUiLogManager() {
    }

    public static NetworkUiLogManager getInstance() {
        if (instance == null) {
            instance = new NetworkUiLogManager();
        }
        return instance;
    }

    public static void append(String msg) {
        getInstance().appendLog(msg);
    }

    static class Log {
        public long time;
        public String message;

        public Log(long time, String message) {
            this.time = time;
            this.message = message;
        }
    }

    public interface OnLogAddedListener {
        void OnLogAdded(long time, String msg);
    }

    public void appendLog(String msg) {
        if (listener != null) {
            listener.OnLogAdded(System.currentTimeMillis(), msg);
        } else {
            queue.add(new Log(System.currentTimeMillis(), msg));
        }
    }

    public void setOnLogAddedListener(OnLogAddedListener listener) {
        this.listener = listener;
        if (queue.size() > 0) {
            flushLogs();
        }
    }

    private void flushLogs() {
        Log log = queue.poll();
        if (log == null) {
            return;
        }
        listener.OnLogAdded(log.time, log.message);

        if (queue.size() > 0) {
            flushLogs();
        }
    }
}
