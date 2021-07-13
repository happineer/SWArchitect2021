package com.lge.cmuteam3.client.ui;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.*;
import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.File;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class LeaningModeDialog extends JDialog {
    private static final Logger LOG = LoggerFactory.getLogger(LeaningModeDialog.class);

    private JTextField textField;
    private File[] files;
    private final JPanel gridPanel;
    private final JLabel imageCountLabel;

    public LeaningModeDialog(JFrame frame, String title, File[] files, OnDialogEventListener listener) {
        super(frame, title, true);
        this.files = files;
        getContentPane().setLayout(new BorderLayout(0, 0));

        JPanel namePanel = new JPanel();
        getContentPane().add(namePanel, BorderLayout.NORTH);

        JLabel nameLabel = new JLabel("Name");
        namePanel.add(nameLabel);

        textField = new JTextField();
        namePanel.add(textField);
        textField.setColumns(10);

        JButton confirmButton = new JButton("Confirm");
        namePanel.add(confirmButton);
        confirmButton.addActionListener(e -> {
            String name = textField.getText().trim();
            LOG.info("name:" + name);
            if (name.length() == 0) {
                JOptionPane.showMessageDialog(LeaningModeDialog.this, "Name is not inserted.");
                return;
            }
            dispose();
            listener.onDialogSend(name, files);
        });

        JButton cancelButton = new JButton("Cancel");
        namePanel.add(cancelButton);
        cancelButton.addActionListener(e -> dispose());

        int rows = files.length / 5 + 1;

        gridPanel = new JPanel();
        getContentPane().add(gridPanel, BorderLayout.CENTER);
        gridPanel.setLayout(new GridLayout(rows, 5, 0, 0));

        JPanel panel = new JPanel();
        getContentPane().add(panel, BorderLayout.SOUTH);

        imageCountLabel = new JLabel("0");
        panel.add(imageCountLabel);
        imageCountLabel.setText(Integer.toString(files.length));

        JLabel imageTextLabel = new JLabel("image(s) selected.");
        panel.add(imageTextLabel);

        addWindowListener(new WindowAdapter() {
            @Override
            public void windowOpened(WindowEvent e) {
                super.windowOpened(e);
                LOG.info("windowOpened");
                addImageTiles();
            }

            @Override
            public void windowClosed(WindowEvent e) {
                super.windowClosed(e);
                LOG.info("windowClosed");
                listener.onDialogCanceled();
            }
        });
    }

    private void addImageTiles() {
        ExecutorService executor = Executors.newSingleThreadExecutor();
        for (File file : files) {
            JLabel label = new JLabel();
            executor.submit(() -> {
                label.setIcon(new ImageIcon(loadAndResizeImage(file)));
                pack();
            });
            gridPanel.add(label);
        }
    }

    private Image loadAndResizeImage(File file) {
        return new ImageIcon(file.getAbsolutePath()).getImage().getScaledInstance(100, 100, Image.SCALE_DEFAULT);
    }

    public interface OnDialogEventListener {
        void onDialogCanceled();
        void onDialogSend(String name, File[] files);
    }
}
