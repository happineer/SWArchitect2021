package com.lge.cmuteam3.client;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.*;

public class TcpClientMain {
    // Slf4j + logback applied.
    private static final Logger LOG = LoggerFactory.getLogger(TcpClientMain.class);

    public static void main(String[] args) {
        LOG.info("Client Application starts");

        // Server ip and port could be moved into Receiver class, but I leave them here to consider later.
        FileProperties prop = FileProperties.getInstance();
        String serverIp = prop.getProperty("server.ip");
        int serverPort = Integer.parseInt(prop.getProperty("server.port"));

        JFrame frame = new JFrame("CMU 3");
        JPanel panel = new JPanel();
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setSize(500, 500);
        frame.setVisible(true);

        Receiver receiver = new Receiver(serverIp, serverPort);
        Player player = new Player(receiver, panel, frame);
        receiver.start();
        player.start();
    }
}
