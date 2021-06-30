package com.lge.cmuteam3.client.ui;

import mode.Mode;

import javax.swing.*;
import java.awt.image.BufferedImage;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;

public class UiController {
    private final BaseFrame frame;
    private final StatisticsPanel statisticsPanel;

    private UiModel uiModel;

    public UiController(BaseFrame frame) {
        this.frame = frame;

        statisticsPanel = frame.getStatisticsPanel();
    }

    public void setModePanel(List<Mode> modeList, OnUiEventListener modeManager) {
        JPanel panel = frame.getTopPanel();
        panel.add(new JLabel("Mode"));

        modeList.forEach((mode) -> {
            JButton button = new JButton(mode.getModeName() + " start");
            button.addActionListener(e -> {
                modeManager.onUiStart(mode);
            });
            panel.add(button);

            JButton button2 = new JButton(mode.getModeName() + " stop");
            button2.addActionListener(e -> {
                modeManager.onUiStop(mode);
            });
            panel.add(button2);
        });
    }

    public void updateServerInfo(String serverIp, int serverTransferPort) {
        this.frame.getServerAddressTextField().setText(serverIp + ":" + serverTransferPort);
    }

    public void appendLog(String message) {
        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd hh:mm:ss.SSS");
        String currentTime = dateFormat.format(new Date());

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
        frame.cleanImage();
        uiModel = new UiModel();
        statisticsPanel.reset();
    }

    public void updateImage(BufferedImage image) {
        frame.updateImage(image);
        uiModel.updateImageAdded();
        statisticsPanel.update(uiModel);
    }
}
