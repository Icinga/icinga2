using System;
using System.IO;
using System.Windows.Forms;
using Microsoft.Win32;

namespace Icinga
{
	static class Program
	{

		public static string Icinga2InstallDir
		{
			get
			{
				RegistryKey rk = Registry.LocalMachine.OpenSubKey("SOFTWARE\\Icinga Development Team\\ICINGA2");

				if (rk == null)
					return "";

				return (string)rk.GetValue("");
			}
		}

		public static void FatalError(Form owner, string message)
		{
			MessageBox.Show(owner, message, "Icinga 2 Setup Wizard", MessageBoxButtons.OK, MessageBoxIcon.Error);
			Application.Exit();
		}

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main()
		{
			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault(false);

			string installDir = Program.Icinga2InstallDir;

			if (installDir == "")
				FatalError(null, "Icinga 2 does not seem to be installed properly.");

			Form form;

			if (File.Exists(installDir + "\\etc\\icinga2\\features-enabled\\api.conf"))
				form = new ServiceStatus();
			else
				form = new SetupWizard();

			Application.Run(form);
		}
	}
}
