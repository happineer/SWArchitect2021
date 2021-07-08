package com.lge.cmuteam3.client.ui;

import com.lge.cmuteam3.client.FileProperties;
import com.lge.cmuteam3.client.network.NetworkUiLogManager;
import mode.Mode;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.*;
import java.awt.image.BufferedImage;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

public class UiController implements NetworkUiLogManager.OnLogAddedListener {
    private static final Logger LOG = LoggerFactory.getLogger(UiController.class);

    private final BaseFrame frame;
    private final StatisticsPanel statisticsPanel;
    private final NetworkUiLogManager networkUiLogManager;

    private UiModel uiModel = new UiModel();
    private final ScheduledExecutorService executor;
    private ScheduledFuture<?> future;

    public UiController(BaseFrame frame) {
        this.frame = frame;

        statisticsPanel = frame.getStatisticsPanel();

        executor = Executors.newSingleThreadScheduledExecutor();

        FileProperties prop = FileProperties.getInstance();
        String serverIp = prop.getProperty("server.ip");
        int serverTransferPort = Integer.parseInt(prop.getProperty("server.transfer.port"));
        updateServerInfo(serverIp, serverTransferPort);

        networkUiLogManager = NetworkUiLogManager.getInstance();
        networkUiLogManager.setOnLogAddedListener(this);
    }

    public void setModePanel(List<Mode> modeList, OnUiEventListener modeManager) {
        JPanel panel = frame.getTopPanel();
        panel.add(new JLabel("Mode"));

        modeList.forEach((mode) -> {
            JButton button = new JButton(mode.getModeName() + " start");
            button.addActionListener(e -> {
                LOG.info("Button clicked :" + mode .getModeName());
                modeManager.onUiStart(mode);
            });
            panel.add(button);

//            JButton button2 = new JButton(mode.getModeName() + " stop");
//            button2.addActionListener(e -> {
//                modeManager.onUiStop(mode);
//            });
//            panel.add(button2);
        });
    }

    public void updateServerInfo(String serverIp, int serverTransferPort) {
        this.frame.getServerAddressTextField().setText(serverIp + ":" + serverTransferPort);
    }

    public void appendLog(String message) {
        appendLog(System.currentTimeMillis(), message);
    }

    public void appendLog(long time, String message) {
        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd hh:mm:ss.SSS");
        String currentTime = dateFormat.format(new Date(time));

        frame.appendLog(currentTime, message);
    }

    /**
     * clear and append
     */
    public void setLog(String message) {
        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd hh:mm:ss.SSS");
        String currentTime = dateFormat.format(new Date());

        clearLog();
        frame.appendLog(currentTime, message);
    }

    public void clearLog() {
        frame.clearLog();
    }

    public void reset() {
        uiModel = new UiModel();

        frame.cleanImage();
        uiModel = new UiModel();
        statisticsPanel.reset();
    }

    public void updateImage(BufferedImage image) {
        uiModel.updateImageAdded();

        frame.updateImage(image);
        statisticsPanel.update(uiModel);
    }

    public void updateHistogram() {
        if (uiModel == null) {
            LOG.info("UiModel is not prepared.");
            return;
        }
        double[] data = uiModel.getHistogramData();
        LOG.info("data size:" + data.length);
        frame.updateChart(data);
    }

    public void runHistogramUpdater() {
    	System.out.println("gaenoo runrun");
        future = executor.scheduleAtFixedRate(this::updateHistogram, 1000,4000, TimeUnit.MILLISECONDS);
    }

    public void stopHistogramUpdater() {
        if (future != null) {
            future.cancel(true);
        }
    }

    @Override
    public void OnLogAdded(long time, String msg) {
        appendLog(time, msg);
    }

    public BaseFrame getMainFrame() {
        return frame;
    }
}
