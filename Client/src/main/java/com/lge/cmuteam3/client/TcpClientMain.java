package com.lge.cmuteam3.client;

import com.lge.cmuteam3.client.network.NetworkManager;
import com.lge.cmuteam3.client.ui.BaseFrame;

import mode.ModeManager;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.*;

public class TcpClientMain {
    // Slf4j + logback applied.
    private static final Logger LOG = LoggerFactory.getLogger(TcpClientMain.class);

    public static void main(String[] args) {
        LOG.info("Client Application starts");

        if (!NetworkManager.getInstance().init()) {
//        	System.exit(0);
        }
        BaseFrame frame = BaseFrame.getInstance();
        frame.setModePanel();
        
        
        frame.setVisible(true);
    }
}
