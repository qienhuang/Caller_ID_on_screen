using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace cider
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            bool isAppRunning = false;
            System.Threading.Mutex mutex = new System.Threading.Mutex(true, "Cider", out isAppRunning);
            if (!isAppRunning)
            {
                MessageBox.Show("Program already running!");
                Environment.Exit(1);
            }
            else
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new MainForm());
            }


        }
    }
}
