package com.lge.cmuteam3.client;

import com.lge.cmuteam3.client.network.OnConnectListener;
import com.lge.cmuteam3.client.ui.UiController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.awt.image.BufferedImage;
import java.net.Socket;
import java.util.Timer;
import java.util.TimerTask;

public class Player implements OnConnectListener {
    private static final Logger LOG = LoggerFactory.getLogger(Player.class);

    // below fields will be loaded by client.properties file
    private String serverIp;
    private int serverTransferPort = 5000;
    private int delay = 80;

    private Receiver receiver;
    private TimerTask task;

    private boolean running = false;
    private final UiController uiController;

    public void stop() {
        LOG.debug("Event: onUiRunModeStop");
        if (task != null) {
            task.cancel();
            task = null;

            running = false;
            if (Player.this.receiver != null) {
                Player.this.receiver.stopSelf();
            }

            Player.this.receiver = null;
            showLog("Disconnected!");
        } else {
            showLog("Not running!");
        }
    }

    private class Scheduler extends TimerTask {

        @Override
        public void run() {
            BufferedImage image = receiver.getImageFrame();
            if (image != null) {
                LOG.debug("image taken. remain buffer : " + receiver.getRemainBufferSize());
                uiController.updateImage(image);
            } else {
                LOG.debug("image is not ready");
                try {
                    Thread.sleep(delay);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public Player(Receiver receiver, UiController uiController) {
        this.uiController = uiController;

        FileProperties prop = FileProperties.getInstance();
        this.serverIp = prop.getProperty("server.ip");
        this.serverTransferPort = Integer.parseInt(prop.getProperty("server.transfer.port"));
        this.delay = Integer.parseInt(prop.getProperty("client.delay"));

        this.receiver = receiver;

        this.uiController.updateServerInfo(serverIp, serverTransferPort);
        this.uiController.reset();
    }

    public void start(Socket socket) {
        playImages(socket);
    }

    private void playImages(Socket socket) {
        if (task != null || running) {
            showLog("Already running!");
            return;
        }

        running = true;
        showLog("Try to connect...");
        receiver = new Receiver(socket);
        receiver.start();

        receiver.setOnConnectListener(this);
    }

    private void startReceiving() {
        task = new Scheduler();
        Timer timer = new Timer();
        timer.schedule(task, 0, delay);
    }

    private void showLog(String message) {
        uiController.appendLog(message);
    }

    @Override
    public void onConnected() {
        showLog("Connected!");
        startReceiving();
    }

    @Override
    public void onFailed() {
        showLog("Connection failed!");
        if (task != null) {
            task.cancel();
            task = null;
        }

        running = false;
    }

}