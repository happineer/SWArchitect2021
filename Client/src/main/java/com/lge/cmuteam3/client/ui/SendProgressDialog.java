package com.lge.cmuteam3.client.ui;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.*;
import javax.swing.border.Border;
import java.awt.*;

public class SendProgressDialog extends JDialog {
    private static final Logger LOG = LoggerFactory.getLogger(SendProgressDialog.class);

    private final JProgressBar progressBar;

    public SendProgressDialog(JFrame frame, String name) {
        super(frame, name, false);

        progressBar = new JProgressBar();
        progressBar.setStringPainted(true);
        Border border = BorderFactory.createTitledBorder("Sending...");
        progressBar.setBorder(border);
        Container contentPane = getContentPane();
        contentPane.add(progressBar, BorderLayout.NORTH);
        setSize(300, 100);
        setVisible(true);
        setLocationRelativeTo(frame);
    }

    public void update(int value) {
        progressBar.setValue(value);
    }
}
