package com.lge.cmuteam3.client;

import com.lge.cmuteam3.client.ui.BaseFrame;
import com.lge.cmuteam3.client.ui.UiController;
import mode.ModeManager;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.awt.EventQueue;
import java.util.concurrent.Executors;

public class TcpClientMain {
    // Slf4j + logback applied.
    private static final Logger LOG = LoggerFactory.getLogger(TcpClientMain.class);

    public static void main(String[] args) {
        LOG.info("Client Application starts");

        EventQueue.invokeLater(() -> {
            BaseFrame frame = new BaseFrame();
            frame.setVisible(true);

            UiController uiController = new UiController(frame);

            Executors.newSingleThreadExecutor().submit(() ->
                    PlaybackManager.initialize(uiController)
            );

            ModeManager modeManager = ModeManager.getInstance();
            modeManager.init(uiController);
        });
    }
}
