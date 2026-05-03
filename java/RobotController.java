import javax.swing.*;
import javax.swing.border.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.net.*;

public class RobotController {

    static Socket         socket;
    static PrintWriter    writer;
    static BufferedReader reader;
    static JLabel         lblStatus;

    public static void main(String[] args) {
        String ip = JOptionPane.showInputDialog(null,
            "Inserisci IP del Raspberry Pi:", "192.168.1.100");
        if (ip == null || ip.isBlank()) ip = "192.168.1.100";
        ip = ip.trim();

        try {
            socket = new Socket(ip, 5000);
            socket.setSoTimeout(2000);
            writer = new PrintWriter(socket.getOutputStream(), true);
            reader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            System.out.println("Connesso a " + ip);
        } catch (Exception ex) {
            JOptionPane.showMessageDialog(null, "Connessione fallita!\n" + ex.getMessage());
            System.exit(1);
        }

        SwingUtilities.invokeLater(RobotController::buildUI);
        Thread t = new Thread(RobotController::readLoop, "reader");
        t.setDaemon(true);
        t.start();
    }

    static void buildUI() {
        JFrame frame = new JFrame("Spider Robot Controller");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setSize(360, 340);
        frame.setResizable(false);
        frame.getContentPane().setBackground(new Color(20, 20, 30));

        JPanel root = new JPanel(new BorderLayout(10, 10));
        root.setBackground(new Color(20, 20, 30));
        root.setBorder(new EmptyBorder(12, 12, 12, 12));

        JLabel title = new JLabel("SPIDER ROBOT", SwingConstants.CENTER);
        title.setFont(new Font("Monospaced", Font.BOLD, 22));
        title.setForeground(new Color(0, 220, 180));
        root.add(title, BorderLayout.NORTH);

        root.add(buildControlPanel(), BorderLayout.CENTER);

        lblStatus = new JLabel("Connesso", SwingConstants.CENTER);
        lblStatus.setForeground(new Color(0, 220, 120));
        lblStatus.setFont(new Font("Monospaced", Font.PLAIN, 11));
        root.add(lblStatus, BorderLayout.SOUTH);

        frame.add(root);
        frame.setVisible(true);

        frame.addKeyListener(new KeyAdapter() {
            public void keyPressed(KeyEvent e) {
                int k = e.getKeyCode();
                if (k == KeyEvent.VK_UP    || k == KeyEvent.VK_W) sendCommand("F");
                if (k == KeyEvent.VK_DOWN  || k == KeyEvent.VK_S) sendCommand("B");
                if (k == KeyEvent.VK_LEFT  || k == KeyEvent.VK_A) sendCommand("L");
                if (k == KeyEvent.VK_RIGHT || k == KeyEvent.VK_D) sendCommand("R");
                if (k == KeyEvent.VK_SPACE)                        sendCommand("S");
            }
        });
        frame.setFocusable(true);
    }

    static JPanel buildControlPanel() {
        JPanel p = new JPanel(new GridBagLayout());
        p.setBackground(new Color(28, 28, 42));
        p.setBorder(BorderFactory.createTitledBorder(
            BorderFactory.createLineBorder(new Color(0, 180, 150), 1),
            "Controllo", TitledBorder.LEFT, TitledBorder.TOP,
            new Font("Monospaced", Font.BOLD, 11), new Color(0, 180, 150)));

        GridBagConstraints g = new GridBagConstraints();
        g.insets  = new Insets(6, 6, 6, 6);
        g.fill    = GridBagConstraints.BOTH;
        g.weightx = 1;
        g.weighty = 1;

        JButton btnForward = makeButton("AVANTI",   new Color(0, 140, 100));
        JButton btnLeft    = makeButton("SINISTRA", new Color(0, 100, 160));
        JButton btnStop    = makeButton("STOP",     new Color(180, 50, 50));
        JButton btnRight   = makeButton("DESTRA",   new Color(0, 100, 160));
        JButton btnBack    = makeButton("INDIETRO", new Color(100, 80, 0));

        btnForward.addActionListener(e -> sendCommand("F"));
        btnLeft   .addActionListener(e -> sendCommand("L"));
        btnStop   .addActionListener(e -> sendCommand("S"));
        btnRight  .addActionListener(e -> sendCommand("R"));
        btnBack   .addActionListener(e -> sendCommand("B"));

        g.gridx = 1; g.gridy = 0; p.add(btnForward, g);
        g.gridx = 0; g.gridy = 1; p.add(btnLeft,    g);
        g.gridx = 1; g.gridy = 1; p.add(btnStop,    g);
        g.gridx = 2; g.gridy = 1; p.add(btnRight,   g);
        g.gridx = 1; g.gridy = 2; p.add(btnBack,    g);
        return p;
    }

    static JButton makeButton(String text, Color bg) {
        JButton b = new JButton(text);
        b.setBackground(bg);
        b.setForeground(Color.WHITE);
        b.setFont(new Font("Monospaced", Font.BOLD, 13));
        b.setFocusPainted(false);
        b.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(bg.brighter(), 1),
            new EmptyBorder(10, 14, 10, 14)));
        b.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        b.addMouseListener(new MouseAdapter() {
            public void mouseEntered(MouseEvent e) { b.setBackground(bg.brighter()); }
            public void mouseExited (MouseEvent e) { b.setBackground(bg); }
        });
        return b;
    }

    static void sendCommand(String cmd) {
        if (writer != null) {
            writer.println(cmd);
            System.out.println("Sent: " + cmd);
        }
    }

    static void readLoop() {
        while (!Thread.currentThread().isInterrupted()) {
            try {
                String line = reader.readLine();
                if (line == null) {
                    SwingUtilities.invokeLater(() -> lblStatus.setText("Disconnesso"));
                    break;
                }
                // ignora tutto (niente sonar display)
            } catch (SocketTimeoutException ex) {
                // normale, continua
            } catch (IOException ex) {
                SwingUtilities.invokeLater(() -> lblStatus.setText("Disconnesso"));
                break;
            }
        }
    }
}