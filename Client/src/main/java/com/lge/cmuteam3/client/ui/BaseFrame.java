package com.lge.cmuteam3.client.ui;

import javax.swing.*;
import javax.swing.border.EmptyBorder;

import java.awt.*;
import java.awt.event.*;
import java.awt.image.BufferedImage;

public class BaseFrame extends JFrame {
	private static final long serialVersionUID = -2860632918949315691L;

	private JPanel contentPane;
	private JLabel imageView;
	private JTextArea logArea;
	private JScrollPane scrollPane;

	private JLabel labelServer;
	private JTextField serverAddressTextField;

	private StatisticsPanel statisticsPanel;
	private JPanel bottomPanel;
	private JPanel jitterPanel;
	private JLabel jitterLabel;

	public JPanel getTopPanel() {
		return topPanel;
	}

	private final JPanel topPanel;

	public JLabel getImageView() {
		return imageView;
	}

	public JTextArea getLogArea() {
		return logArea;
	}

	public JTextField getServerAddressTextField() {
		return serverAddressTextField;
	}

	public StatisticsPanel getStatisticsPanel() {
		return statisticsPanel;
	}

	public BaseFrame() {
		setTitle("CMU LG SW Architect Team 3");
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		setBounds(100, 100, 750, 900);
		contentPane = new JPanel();
		contentPane.setBorder(new EmptyBorder(5, 5, 5, 5));
		contentPane.setLayout(new BorderLayout(0, 0));
		setContentPane(contentPane);

		imageView = new JLabel("");
		imageView.setSize(1280, 720);
		contentPane.add(imageView, BorderLayout.CENTER);


		topPanel = new JPanel();
		contentPane.add(topPanel, BorderLayout.NORTH);

		labelServer = new JLabel("Target Server");
		topPanel.add(labelServer);

		serverAddressTextField = new JTextField();
		topPanel.add(serverAddressTextField);
		serverAddressTextField.setColumns(12);
		serverAddressTextField.setEditable(false);

		logArea = new JTextArea();
		logArea.setRows(10);
		logArea.setEditable(false);


		bottomPanel = new JPanel();
		contentPane.add(bottomPanel, BorderLayout.SOUTH);
		bottomPanel.setLayout(new BoxLayout(bottomPanel, BoxLayout.X_AXIS));

		scrollPane = new JScrollPane(logArea);
		bottomPanel.add(scrollPane, BorderLayout.SOUTH);

		jitterPanel = new JPanel();
		bottomPanel.add(jitterPanel);

		jitterLabel = new JLabel("Jitter Histogram");
		jitterLabel.setHorizontalAlignment(SwingConstants.LEFT);
		jitterPanel.add(jitterLabel);

		statisticsPanel = new StatisticsPanel();
		contentPane.add(statisticsPanel, BorderLayout.WEST);

		// call onCancel() when cross is clicked
		setDefaultCloseOperation(DO_NOTHING_ON_CLOSE);
		addWindowListener(new WindowAdapter() {
			public void windowClosing(WindowEvent e) {
				onCancel();
			}
		});

		// call onCancel() on ESCAPE
		contentPane.registerKeyboardAction(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				onCancel();
			}
		}, KeyStroke.getKeyStroke(KeyEvent.VK_ESCAPE, 0), JComponent.WHEN_ANCESTOR_OF_FOCUSED_COMPONENT);
	}
	
	private void onCancel() {
		// add your code here if necessary
		dispose();
	}

	void updateImage(BufferedImage image) {
		imageView.setIcon(new ImageIcon(image));
		pack();
	}

	void cleanImage() {
		imageView.setIcon(null);
	}

	void appendLog(String currentTime, String message) {
		logArea.append(String.format("[%s] %s\n", currentTime, message));
		logArea.setCaretPosition(logArea.getDocument().getLength());
	}

	void clearLog() {
		logArea.replaceRange("", 0, logArea.getDocument().getLength());
	}
}
