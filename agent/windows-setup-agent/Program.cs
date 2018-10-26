using System;
using System.IO;
using System.Windows.Forms;
using Microsoft.Win32;
using System.Runtime.InteropServices;
using System.Text;

namespace Icinga
{
    internal static class NativeMethods
    {
        [DllImport("msi.dll", CharSet = CharSet.Unicode)]
        internal static extern int MsiEnumProducts(int iProductIndex, StringBuilder lpProductBuf);

        [DllImport("msi.dll", CharSet = CharSet.Unicode)]
        internal static extern Int32 MsiGetProductInfo(string product, string property, [Out] StringBuilder valueBuf, ref Int32 len);
    }

    static class Program
	{
		public static string Icinga2InstallDir
		{
			get
			{
				StringBuilder szProduct;

				for (int index = 0; ; index++) {
					szProduct = new StringBuilder(39);
					if (NativeMethods.MsiEnumProducts(index, szProduct) != 0)
						break;

					int cbName = 128;
					StringBuilder szName = new StringBuilder(cbName);

					if (NativeMethods.MsiGetProductInfo(szProduct.ToString(), "ProductName", szName, ref cbName) != 0)
						continue;

					if (szName.ToString() != "Icinga 2")
						continue;

					int cbLocation = 1024;
					StringBuilder szLocation = new StringBuilder(cbLocation);
					if (NativeMethods.MsiGetProductInfo(szProduct.ToString(), "InstallLocation", szLocation, ref cbLocation) == 0)
						return szLocation.ToString();
				}

				return "";
			}
		}

		public static string Icinga2DataDir
		{
			get
			{
				return Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData) + "\\icinga2";
			}
		}

		public static string Icinga2User
		{
			get
			{
				if (!File.Exists(Icinga2DataDir + "\\etc\\icinga2\\user"))
					return "NT AUTHORITY\\NetworkService";
				System.IO.StreamReader file = new System.IO.StreamReader(Icinga2DataDir + "\\etc\\icinga2\\user");
				string line = file.ReadLine();
				file.Close();

				if (line != null)
					return line;
				else
					return "NT AUTHORITY\\NetworkService";
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

			if (installDir == "") {
				FatalError(null, "Icinga 2 does not seem to be installed properly.");
				return;
			}

			Form form;

			if (File.Exists(Program.Icinga2DataDir + "\\etc\\icinga2\\features-enabled\\api.conf"))
				form = new ServiceStatus();
			else
				form = new SetupWizard();

			Application.Run(form);
		}
	}
}
