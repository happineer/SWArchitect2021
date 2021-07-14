package com.lge.cmuteam3.client.ui;

import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartPanel;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.data.statistics.HistogramDataset;
import org.jfree.data.statistics.HistogramType;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.*;
import javax.swing.border.EmptyBorder;

import java.awt.*;
import java.awt.event.*;
import java.awt.image.BufferedImage;

public class BaseFrame extends JFrame {
	private static final Logger LOG = LoggerFactory.getLogger(BaseFrame.class);

	private static final long serialVersionUID = -2860632918949315691L;

	private JPanel contentPane;
	private JLabel imageView;
	private JTextArea logArea;
	private JScrollPane scrollPane;

	private JLabel labelServer;
	private JLabel labelConnection;
	private JTextField serverAddressTextField;
	private JTextField ConnectionStatusTextField;

	private StatisticsPanel statisticsPanel;
	private JPanel bottomPanel;
	private JPanel jitterPanel;

	public JPanel getModePanel() {
		return modePanel;
	}

	private final JPanel topPanel;
	private final JPanel statusPanel;
	private final JPanel modePanel;
	
	private JPanel chartPanel;
	private JPanel bottomLeftPanel;
	private JLabel logLabel;

	public JLabel getImageView() {
		return imageView;
	}

	public JTextArea getLogArea() {
		return logArea;
	}

	public JTextField getConnectionStatusTextField() {
		return ConnectionStatusTextField;
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
		setBounds(100, 100, 1088, 916);
		contentPane = new JPanel();
		contentPane.setBorder(new EmptyBorder(5, 5, 5, 5));
		contentPane.setLayout(new BorderLayout(0, 0));
		setContentPane(contentPane);

		imageView = new JLabel("");
		imageView.setSize(960, 540);
		contentPane.add(imageView, BorderLayout.CENTER);

		topPanel = new JPanel();
		contentPane.add(topPanel, BorderLayout.NORTH);
		topPanel.setLayout(new BoxLayout(topPanel, BoxLayout.Y_AXIS));
		
		statusPanel = new JPanel();
		topPanel.add(statusPanel, BorderLayout.NORTH);
		modePanel = new JPanel();
		topPanel.add(modePanel, BorderLayout.NORTH);

		labelConnection = new JLabel("Server connection");
		labelConnection.setFont(new Font("Lucida Sans", Font.PLAIN, 16));
		statusPanel.add(labelConnection);
		
		ConnectionStatusTextField = new JTextField();
		ConnectionStatusTextField.setFont(new Font("Lucida Sans", Font.PLAIN, 16));
		statusPanel.add(ConnectionStatusTextField);
		ConnectionStatusTextField.setColumns(15);
		ConnectionStatusTextField.setEditable(false);
		
		
		labelServer = new JLabel("Target Server");
		labelServer.setFont(new Font("Lucida Sans", Font.PLAIN, 16));
		statusPanel.add(labelServer);

		serverAddressTextField = new JTextField();
		serverAddressTextField.setFont(new Font("Lucida Sans", Font.PLAIN, 16));
		statusPanel.add(serverAddressTextField);
		serverAddressTextField.setColumns(15);
		serverAddressTextField.setEditable(false);

		logArea = new JTextArea();
		logArea.setBackground(Color.WHITE);
		logArea.setRows(10);
		logArea.setEditable(false);
		logArea.setFont(new Font("Lucida Sans", Font.PLAIN, 15));


		bottomPanel = new JPanel();
		contentPane.add(bottomPanel, BorderLayout.SOUTH);
		bottomPanel.setLayout(new BoxLayout(bottomPanel, BoxLayout.X_AXIS));

	
		bottomLeftPanel = new JPanel();
		bottomLeftPanel.setBackground(Color.WHITE);
		bottomPanel.add(bottomLeftPanel);
		bottomLeftPanel.setLayout(new BorderLayout(0, 0));

		scrollPane = new JScrollPane(logArea);
		bottomLeftPanel.add(scrollPane, BorderLayout.CENTER);
		
		logLabel = new JLabel("Controller Log");
		logLabel.setFont(new Font("Arial Black", Font.PLAIN, 20));
		logLabel.setHorizontalAlignment(SwingConstants.CENTER);
		bottomLeftPanel.add(logLabel, BorderLayout.NORTH);
		
		jitterPanel = new JPanel();
		bottomPanel.add(jitterPanel);
		jitterPanel.setLayout(new BoxLayout(jitterPanel, BoxLayout.X_AXIS));

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

		addWindowListener(new WindowAdapter() {
			@Override
			public void windowClosed(WindowEvent e) {
				super.windowClosed(e);
				LOG.info("windowClosed, exit app");
				System.exit(0);
			}
		});
	}

	private void onCancel() {
		// add your code here if necessary
		dispose();
	}

	void updateImage(BufferedImage image) {
		imageView.setIcon(new ImageIcon(image));
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

	void updateChart(double[] data) {
		if (data == null || data.length == 0) {
			LOG.info("No data to display on the chart.");
			return;
		}

		HistogramDataset dataset = new HistogramDataset();
		dataset.setType(HistogramType.FREQUENCY);
		dataset.addSeries("Hist", data, 30);

		JFreeChart chart = ChartFactory.createHistogram("Jitter Histogram", "Jitter (ms)", "Frames", dataset, PlotOrientation.VERTICAL,
				false, false, false);

		EventQueue.invokeLater(() -> {
			ChartPanel panel = new ChartPanel(chart);
			panel.setPreferredSize(new Dimension(550, 230));

			jitterPanel.removeAll();
			jitterPanel.add(panel);
			jitterPanel.revalidate();
		});
	}
}
