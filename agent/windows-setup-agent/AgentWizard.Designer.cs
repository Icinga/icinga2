namespace Icinga
{
	partial class AgentWizard
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AgentWizard));
			this.tbcPages = new System.Windows.Forms.TabControl();
			this.tabAgentKey = new System.Windows.Forms.TabPage();
			this.lblHostKey = new System.Windows.Forms.Label();
			this.prgHostKey = new System.Windows.Forms.ProgressBar();
			this.tabCSR = new System.Windows.Forms.TabPage();
			this.txtCSR = new System.Windows.Forms.TextBox();
			this.lblCSRPrompt = new System.Windows.Forms.Label();
			this.tabCertificateBundle = new System.Windows.Forms.TabPage();
			this.txtBundle = new System.Windows.Forms.TextBox();
			this.lblBundlePrompt = new System.Windows.Forms.Label();
			this.tabParameters = new System.Windows.Forms.TabPage();
			this.txtInstanceName = new System.Windows.Forms.TextBox();
			this.label5 = new System.Windows.Forms.Label();
			this.groupBox3 = new System.Windows.Forms.GroupBox();
			this.rdoNoConnect = new System.Windows.Forms.RadioButton();
			this.txtPeerPort = new System.Windows.Forms.TextBox();
			this.lblPeerPort = new System.Windows.Forms.Label();
			this.txtPeerHost = new System.Windows.Forms.TextBox();
			this.lblPeerHost = new System.Windows.Forms.Label();
			this.rdoConnect = new System.Windows.Forms.RadioButton();
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.rdoNoListener = new System.Windows.Forms.RadioButton();
			this.txtListenerPort = new System.Windows.Forms.TextBox();
			this.lblListenerPort = new System.Windows.Forms.Label();
			this.rdoListener = new System.Windows.Forms.RadioButton();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.txtMasterInstance = new System.Windows.Forms.TextBox();
			this.lblMasterInstance = new System.Windows.Forms.Label();
			this.rdoNoMaster = new System.Windows.Forms.RadioButton();
			this.rdoNewMaster = new System.Windows.Forms.RadioButton();
			this.picBanner = new System.Windows.Forms.PictureBox();
			this.btnBack = new System.Windows.Forms.Button();
			this.btnNext = new System.Windows.Forms.Button();
			this.btnCancel = new System.Windows.Forms.Button();
			this.tabConfigure = new System.Windows.Forms.TabPage();
			this.prgConfig = new System.Windows.Forms.ProgressBar();
			this.lblConfigStatus = new System.Windows.Forms.Label();
			this.tabFinish = new System.Windows.Forms.TabPage();
			this.label1 = new System.Windows.Forms.Label();
			this.tbcPages.SuspendLayout();
			this.tabAgentKey.SuspendLayout();
			this.tabCSR.SuspendLayout();
			this.tabCertificateBundle.SuspendLayout();
			this.tabParameters.SuspendLayout();
			this.groupBox3.SuspendLayout();
			this.groupBox2.SuspendLayout();
			this.groupBox1.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.picBanner)).BeginInit();
			this.tabConfigure.SuspendLayout();
			this.tabFinish.SuspendLayout();
			this.SuspendLayout();
			// 
			// tbcPages
			// 
			this.tbcPages.Appearance = System.Windows.Forms.TabAppearance.FlatButtons;
			this.tbcPages.Controls.Add(this.tabAgentKey);
			this.tbcPages.Controls.Add(this.tabCSR);
			this.tbcPages.Controls.Add(this.tabCertificateBundle);
			this.tbcPages.Controls.Add(this.tabParameters);
			this.tbcPages.Controls.Add(this.tabConfigure);
			this.tbcPages.Controls.Add(this.tabFinish);
			this.tbcPages.ItemSize = new System.Drawing.Size(0, 1);
			this.tbcPages.Location = new System.Drawing.Point(0, 80);
			this.tbcPages.Margin = new System.Windows.Forms.Padding(0);
			this.tbcPages.Name = "tbcPages";
			this.tbcPages.SelectedIndex = 0;
			this.tbcPages.Size = new System.Drawing.Size(625, 509);
			this.tbcPages.SizeMode = System.Windows.Forms.TabSizeMode.Fixed;
			this.tbcPages.TabIndex = 0;
			this.tbcPages.SelectedIndexChanged += new System.EventHandler(this.tbcPages_SelectedIndexChanged);
			// 
			// tabAgentKey
			// 
			this.tabAgentKey.Controls.Add(this.lblHostKey);
			this.tabAgentKey.Controls.Add(this.prgHostKey);
			this.tabAgentKey.Location = new System.Drawing.Point(4, 5);
			this.tabAgentKey.Name = "tabAgentKey";
			this.tabAgentKey.Padding = new System.Windows.Forms.Padding(3);
			this.tabAgentKey.Size = new System.Drawing.Size(617, 500);
			this.tabAgentKey.TabIndex = 0;
			this.tabAgentKey.Text = "Agent Key";
			this.tabAgentKey.UseVisualStyleBackColor = true;
			// 
			// lblHostKey
			// 
			this.lblHostKey.AutoSize = true;
			this.lblHostKey.Location = new System.Drawing.Point(118, 222);
			this.lblHostKey.Name = "lblHostKey";
			this.lblHostKey.Size = new System.Drawing.Size(197, 13);
			this.lblHostKey.TabIndex = 1;
			this.lblHostKey.Text = "Generating a host key for this machine...";
			// 
			// prgHostKey
			// 
			this.prgHostKey.Location = new System.Drawing.Point(118, 254);
			this.prgHostKey.Name = "prgHostKey";
			this.prgHostKey.Size = new System.Drawing.Size(369, 23);
			this.prgHostKey.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
			this.prgHostKey.TabIndex = 0;
			// 
			// tabCSR
			// 
			this.tabCSR.Controls.Add(this.txtCSR);
			this.tabCSR.Controls.Add(this.lblCSRPrompt);
			this.tabCSR.Location = new System.Drawing.Point(4, 5);
			this.tabCSR.Name = "tabCSR";
			this.tabCSR.Padding = new System.Windows.Forms.Padding(3);
			this.tabCSR.Size = new System.Drawing.Size(617, 500);
			this.tabCSR.TabIndex = 1;
			this.tabCSR.Text = "Certificate Signing Request";
			this.tabCSR.UseVisualStyleBackColor = true;
			// 
			// txtCSR
			// 
			this.txtCSR.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.txtCSR.Location = new System.Drawing.Point(24, 42);
			this.txtCSR.Multiline = true;
			this.txtCSR.Name = "txtCSR";
			this.txtCSR.ReadOnly = true;
			this.txtCSR.Size = new System.Drawing.Size(564, 452);
			this.txtCSR.TabIndex = 1;
			// 
			// lblCSRPrompt
			// 
			this.lblCSRPrompt.AutoSize = true;
			this.lblCSRPrompt.Location = new System.Drawing.Point(21, 15);
			this.lblCSRPrompt.Name = "lblCSRPrompt";
			this.lblCSRPrompt.Size = new System.Drawing.Size(373, 13);
			this.lblCSRPrompt.TabIndex = 0;
			this.lblCSRPrompt.Text = "Please sign the following certificate signing request (CSR) using the agent CA:";
			// 
			// tabCertificateBundle
			// 
			this.tabCertificateBundle.Controls.Add(this.txtBundle);
			this.tabCertificateBundle.Controls.Add(this.lblBundlePrompt);
			this.tabCertificateBundle.Location = new System.Drawing.Point(4, 5);
			this.tabCertificateBundle.Name = "tabCertificateBundle";
			this.tabCertificateBundle.Padding = new System.Windows.Forms.Padding(3);
			this.tabCertificateBundle.Size = new System.Drawing.Size(617, 500);
			this.tabCertificateBundle.TabIndex = 2;
			this.tabCertificateBundle.Text = "Certificate Bundle";
			this.tabCertificateBundle.UseVisualStyleBackColor = true;
			// 
			// txtBundle
			// 
			this.txtBundle.Font = new System.Drawing.Font("Courier New", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.txtBundle.Location = new System.Drawing.Point(24, 42);
			this.txtBundle.Multiline = true;
			this.txtBundle.Name = "txtBundle";
			this.txtBundle.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
			this.txtBundle.Size = new System.Drawing.Size(564, 452);
			this.txtBundle.TabIndex = 1;
			// 
			// lblBundlePrompt
			// 
			this.lblBundlePrompt.AutoSize = true;
			this.lblBundlePrompt.Location = new System.Drawing.Point(21, 15);
			this.lblBundlePrompt.Name = "lblBundlePrompt";
			this.lblBundlePrompt.Size = new System.Drawing.Size(239, 13);
			this.lblBundlePrompt.TabIndex = 0;
			this.lblBundlePrompt.Text = "Paste the certificate bundle in the text box below:";
			// 
			// tabParameters
			// 
			this.tabParameters.Controls.Add(this.txtInstanceName);
			this.tabParameters.Controls.Add(this.label5);
			this.tabParameters.Controls.Add(this.groupBox3);
			this.tabParameters.Controls.Add(this.groupBox2);
			this.tabParameters.Controls.Add(this.groupBox1);
			this.tabParameters.Location = new System.Drawing.Point(4, 5);
			this.tabParameters.Name = "tabParameters";
			this.tabParameters.Padding = new System.Windows.Forms.Padding(3);
			this.tabParameters.Size = new System.Drawing.Size(617, 500);
			this.tabParameters.TabIndex = 3;
			this.tabParameters.Text = "Agent Parameters";
			this.tabParameters.UseVisualStyleBackColor = true;
			// 
			// txtInstanceName
			// 
			this.txtInstanceName.Location = new System.Drawing.Point(98, 16);
			this.txtInstanceName.Name = "txtInstanceName";
			this.txtInstanceName.ReadOnly = true;
			this.txtInstanceName.Size = new System.Drawing.Size(240, 20);
			this.txtInstanceName.TabIndex = 0;
			// 
			// label5
			// 
			this.label5.AutoSize = true;
			this.label5.Location = new System.Drawing.Point(9, 20);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(82, 13);
			this.label5.TabIndex = 3;
			this.label5.Text = "Instance Name:";
			// 
			// groupBox3
			// 
			this.groupBox3.Controls.Add(this.rdoNoConnect);
			this.groupBox3.Controls.Add(this.txtPeerPort);
			this.groupBox3.Controls.Add(this.lblPeerPort);
			this.groupBox3.Controls.Add(this.txtPeerHost);
			this.groupBox3.Controls.Add(this.lblPeerHost);
			this.groupBox3.Controls.Add(this.rdoConnect);
			this.groupBox3.Location = new System.Drawing.Point(8, 305);
			this.groupBox3.Name = "groupBox3";
			this.groupBox3.Size = new System.Drawing.Size(601, 140);
			this.groupBox3.TabIndex = 3;
			this.groupBox3.TabStop = false;
			this.groupBox3.Text = "TCP Connect";
			// 
			// rdoNoConnect
			// 
			this.rdoNoConnect.AutoSize = true;
			this.rdoNoConnect.Checked = true;
			this.rdoNoConnect.Location = new System.Drawing.Point(11, 108);
			this.rdoNoConnect.Name = "rdoNoConnect";
			this.rdoNoConnect.Size = new System.Drawing.Size(209, 17);
			this.rdoNoConnect.TabIndex = 3;
			this.rdoNoConnect.TabStop = true;
			this.rdoNoConnect.Text = "Do not connect to the master instance.";
			this.rdoNoConnect.UseVisualStyleBackColor = true;
			this.rdoNoConnect.CheckedChanged += new System.EventHandler(this.RadioConnect_CheckedChanged);
			// 
			// txtPeerPort
			// 
			this.txtPeerPort.Enabled = false;
			this.txtPeerPort.Location = new System.Drawing.Point(131, 79);
			this.txtPeerPort.Name = "txtPeerPort";
			this.txtPeerPort.Size = new System.Drawing.Size(84, 20);
			this.txtPeerPort.TabIndex = 2;
			this.txtPeerPort.Text = "5665";
			// 
			// lblPeerPort
			// 
			this.lblPeerPort.AutoSize = true;
			this.lblPeerPort.Location = new System.Drawing.Point(45, 82);
			this.lblPeerPort.Name = "lblPeerPort";
			this.lblPeerPort.Size = new System.Drawing.Size(29, 13);
			this.lblPeerPort.TabIndex = 6;
			this.lblPeerPort.Text = "Port:";
			// 
			// txtPeerHost
			// 
			this.txtPeerHost.Enabled = false;
			this.txtPeerHost.Location = new System.Drawing.Point(131, 53);
			this.txtPeerHost.Name = "txtPeerHost";
			this.txtPeerHost.Size = new System.Drawing.Size(240, 20);
			this.txtPeerHost.TabIndex = 1;
			// 
			// lblPeerHost
			// 
			this.lblPeerHost.AutoSize = true;
			this.lblPeerHost.Location = new System.Drawing.Point(45, 54);
			this.lblPeerHost.Name = "lblPeerHost";
			this.lblPeerHost.Size = new System.Drawing.Size(58, 13);
			this.lblPeerHost.TabIndex = 1;
			this.lblPeerHost.Text = "Hostname:";
			// 
			// rdoConnect
			// 
			this.rdoConnect.AutoSize = true;
			this.rdoConnect.Location = new System.Drawing.Point(11, 25);
			this.rdoConnect.Name = "rdoConnect";
			this.rdoConnect.Size = new System.Drawing.Size(175, 17);
			this.rdoConnect.TabIndex = 0;
			this.rdoConnect.Text = "Connect to the master instance:";
			this.rdoConnect.UseVisualStyleBackColor = true;
			this.rdoConnect.CheckedChanged += new System.EventHandler(this.RadioConnect_CheckedChanged);
			// 
			// groupBox2
			// 
			this.groupBox2.Controls.Add(this.rdoNoListener);
			this.groupBox2.Controls.Add(this.txtListenerPort);
			this.groupBox2.Controls.Add(this.lblListenerPort);
			this.groupBox2.Controls.Add(this.rdoListener);
			this.groupBox2.Location = new System.Drawing.Point(8, 178);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(601, 111);
			this.groupBox2.TabIndex = 2;
			this.groupBox2.TabStop = false;
			this.groupBox2.Text = "TCP Listener";
			// 
			// rdoNoListener
			// 
			this.rdoNoListener.AutoSize = true;
			this.rdoNoListener.Checked = true;
			this.rdoNoListener.Location = new System.Drawing.Point(11, 82);
			this.rdoNoListener.Name = "rdoNoListener";
			this.rdoNoListener.Size = new System.Drawing.Size(163, 17);
			this.rdoNoListener.TabIndex = 2;
			this.rdoNoListener.TabStop = true;
			this.rdoNoListener.Text = "Do not listen for connections.";
			this.rdoNoListener.UseVisualStyleBackColor = true;
			this.rdoNoListener.CheckedChanged += new System.EventHandler(this.RadioListener_CheckedChanged);
			// 
			// txtListenerPort
			// 
			this.txtListenerPort.Enabled = false;
			this.txtListenerPort.Location = new System.Drawing.Point(132, 51);
			this.txtListenerPort.Name = "txtListenerPort";
			this.txtListenerPort.Size = new System.Drawing.Size(84, 20);
			this.txtListenerPort.TabIndex = 1;
			this.txtListenerPort.Text = "5665";
			// 
			// lblListenerPort
			// 
			this.lblListenerPort.AutoSize = true;
			this.lblListenerPort.Location = new System.Drawing.Point(43, 55);
			this.lblListenerPort.Name = "lblListenerPort";
			this.lblListenerPort.Size = new System.Drawing.Size(29, 13);
			this.lblListenerPort.TabIndex = 1;
			this.lblListenerPort.Text = "Port:";
			// 
			// rdoListener
			// 
			this.rdoListener.AutoSize = true;
			this.rdoListener.Location = new System.Drawing.Point(11, 24);
			this.rdoListener.Name = "rdoListener";
			this.rdoListener.Size = new System.Drawing.Size(250, 17);
			this.rdoListener.TabIndex = 0;
			this.rdoListener.Text = "Listen for connections from the master instance:";
			this.rdoListener.UseVisualStyleBackColor = true;
			this.rdoListener.CheckedChanged += new System.EventHandler(this.RadioListener_CheckedChanged);
			// 
			// groupBox1
			// 
			this.groupBox1.Controls.Add(this.txtMasterInstance);
			this.groupBox1.Controls.Add(this.lblMasterInstance);
			this.groupBox1.Controls.Add(this.rdoNoMaster);
			this.groupBox1.Controls.Add(this.rdoNewMaster);
			this.groupBox1.Location = new System.Drawing.Point(8, 48);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(601, 112);
			this.groupBox1.TabIndex = 1;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Master Instance";
			// 
			// txtMasterInstance
			// 
			this.txtMasterInstance.Location = new System.Drawing.Point(132, 78);
			this.txtMasterInstance.Name = "txtMasterInstance";
			this.txtMasterInstance.Size = new System.Drawing.Size(240, 20);
			this.txtMasterInstance.TabIndex = 2;
			// 
			// lblMasterInstance
			// 
			this.lblMasterInstance.AutoSize = true;
			this.lblMasterInstance.Location = new System.Drawing.Point(40, 81);
			this.lblMasterInstance.Name = "lblMasterInstance";
			this.lblMasterInstance.Size = new System.Drawing.Size(86, 13);
			this.lblMasterInstance.TabIndex = 2;
			this.lblMasterInstance.Text = "Master Instance:";
			// 
			// rdoNoMaster
			// 
			this.rdoNoMaster.AutoSize = true;
			this.rdoNoMaster.Checked = true;
			this.rdoNoMaster.Location = new System.Drawing.Point(11, 50);
			this.rdoNoMaster.Name = "rdoNoMaster";
			this.rdoNoMaster.Size = new System.Drawing.Size(383, 17);
			this.rdoNoMaster.TabIndex = 1;
			this.rdoNoMaster.TabStop = true;
			this.rdoNoMaster.Text = "This instance should report its check results to an existing Icinga 2 instance:";
			this.rdoNoMaster.UseVisualStyleBackColor = true;
			this.rdoNoMaster.CheckedChanged += new System.EventHandler(this.RadioMaster_CheckedChanged);
			// 
			// rdoNewMaster
			// 
			this.rdoNewMaster.AutoSize = true;
			this.rdoNewMaster.Location = new System.Drawing.Point(11, 22);
			this.rdoNewMaster.Name = "rdoNewMaster";
			this.rdoNewMaster.Size = new System.Drawing.Size(167, 17);
			this.rdoNewMaster.TabIndex = 0;
			this.rdoNewMaster.TabStop = true;
			this.rdoNewMaster.Text = "This is a new master instance.";
			this.rdoNewMaster.UseVisualStyleBackColor = true;
			this.rdoNewMaster.CheckedChanged += new System.EventHandler(this.RadioMaster_CheckedChanged);
			// 
			// picBanner
			// 
			this.picBanner.Image = global::Icinga.Properties.Resources.icinga_banner;
			this.picBanner.Location = new System.Drawing.Point(0, 0);
			this.picBanner.Name = "picBanner";
			this.picBanner.Size = new System.Drawing.Size(625, 77);
			this.picBanner.TabIndex = 1;
			this.picBanner.TabStop = false;
			// 
			// btnBack
			// 
			this.btnBack.Enabled = false;
			this.btnBack.Location = new System.Drawing.Point(367, 592);
			this.btnBack.Name = "btnBack";
			this.btnBack.Size = new System.Drawing.Size(75, 23);
			this.btnBack.TabIndex = 1;
			this.btnBack.Text = "< &Back";
			this.btnBack.UseVisualStyleBackColor = true;
			this.btnBack.Click += new System.EventHandler(this.btnBack_Click);
			// 
			// btnNext
			// 
			this.btnNext.Enabled = false;
			this.btnNext.Location = new System.Drawing.Point(448, 592);
			this.btnNext.Name = "btnNext";
			this.btnNext.Size = new System.Drawing.Size(75, 23);
			this.btnNext.TabIndex = 2;
			this.btnNext.Text = "&Next >";
			this.btnNext.UseVisualStyleBackColor = true;
			this.btnNext.Click += new System.EventHandler(this.btnNext_Click);
			// 
			// btnCancel
			// 
			this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.btnCancel.Location = new System.Drawing.Point(538, 592);
			this.btnCancel.Name = "btnCancel";
			this.btnCancel.Size = new System.Drawing.Size(75, 23);
			this.btnCancel.TabIndex = 3;
			this.btnCancel.Text = "Cancel";
			this.btnCancel.UseVisualStyleBackColor = true;
			this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
			// 
			// tabConfigure
			// 
			this.tabConfigure.Controls.Add(this.lblConfigStatus);
			this.tabConfigure.Controls.Add(this.prgConfig);
			this.tabConfigure.Location = new System.Drawing.Point(4, 5);
			this.tabConfigure.Name = "tabConfigure";
			this.tabConfigure.Padding = new System.Windows.Forms.Padding(3);
			this.tabConfigure.Size = new System.Drawing.Size(617, 500);
			this.tabConfigure.TabIndex = 4;
			this.tabConfigure.Text = "Configure Icinga 2";
			this.tabConfigure.UseVisualStyleBackColor = true;
			// 
			// prgConfig
			// 
			this.prgConfig.Location = new System.Drawing.Point(184, 223);
			this.prgConfig.Name = "prgConfig";
			this.prgConfig.Size = new System.Drawing.Size(289, 23);
			this.prgConfig.TabIndex = 0;
			// 
			// lblConfigStatus
			// 
			this.lblConfigStatus.AutoSize = true;
			this.lblConfigStatus.Location = new System.Drawing.Point(184, 204);
			this.lblConfigStatus.Name = "lblConfigStatus";
			this.lblConfigStatus.Size = new System.Drawing.Size(141, 13);
			this.lblConfigStatus.TabIndex = 1;
			this.lblConfigStatus.Text = "Updating the configuration...";
			// 
			// tabFinish
			// 
			this.tabFinish.Controls.Add(this.label1);
			this.tabFinish.Location = new System.Drawing.Point(4, 5);
			this.tabFinish.Name = "tabFinish";
			this.tabFinish.Padding = new System.Windows.Forms.Padding(3);
			this.tabFinish.Size = new System.Drawing.Size(617, 500);
			this.tabFinish.TabIndex = 5;
			this.tabFinish.Text = "Finish";
			this.tabFinish.UseVisualStyleBackColor = true;
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(34, 35);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(214, 13);
			this.label1.TabIndex = 0;
			this.label1.Text = "The Icinga 2 agent was set up successfully.";
			// 
			// AgentWizard
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(625, 624);
			this.Controls.Add(this.btnCancel);
			this.Controls.Add(this.btnNext);
			this.Controls.Add(this.btnBack);
			this.Controls.Add(this.picBanner);
			this.Controls.Add(this.tbcPages);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.MaximizeBox = false;
			this.Name = "AgentWizard";
			this.Text = "Icinga 2 Agent Wizard";
			this.Shown += new System.EventHandler(this.AgentWizard_Shown);
			this.tbcPages.ResumeLayout(false);
			this.tabAgentKey.ResumeLayout(false);
			this.tabAgentKey.PerformLayout();
			this.tabCSR.ResumeLayout(false);
			this.tabCSR.PerformLayout();
			this.tabCertificateBundle.ResumeLayout(false);
			this.tabCertificateBundle.PerformLayout();
			this.tabParameters.ResumeLayout(false);
			this.tabParameters.PerformLayout();
			this.groupBox3.ResumeLayout(false);
			this.groupBox3.PerformLayout();
			this.groupBox2.ResumeLayout(false);
			this.groupBox2.PerformLayout();
			this.groupBox1.ResumeLayout(false);
			this.groupBox1.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.picBanner)).EndInit();
			this.tabConfigure.ResumeLayout(false);
			this.tabConfigure.PerformLayout();
			this.tabFinish.ResumeLayout(false);
			this.tabFinish.PerformLayout();
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.TabControl tbcPages;
		private System.Windows.Forms.TabPage tabAgentKey;
		private System.Windows.Forms.TabPage tabCSR;
		private System.Windows.Forms.PictureBox picBanner;
		private System.Windows.Forms.Button btnBack;
		private System.Windows.Forms.Button btnNext;
		private System.Windows.Forms.Button btnCancel;
		private System.Windows.Forms.Label lblHostKey;
		private System.Windows.Forms.ProgressBar prgHostKey;
		private System.Windows.Forms.TextBox txtCSR;
		private System.Windows.Forms.Label lblCSRPrompt;
		private System.Windows.Forms.TabPage tabCertificateBundle;
		private System.Windows.Forms.TextBox txtBundle;
		private System.Windows.Forms.Label lblBundlePrompt;
		private System.Windows.Forms.TabPage tabParameters;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.TextBox txtMasterInstance;
		private System.Windows.Forms.Label lblMasterInstance;
		private System.Windows.Forms.RadioButton rdoNoMaster;
		private System.Windows.Forms.RadioButton rdoNewMaster;
		private System.Windows.Forms.GroupBox groupBox2;
		private System.Windows.Forms.TextBox txtListenerPort;
		private System.Windows.Forms.Label lblListenerPort;
		private System.Windows.Forms.RadioButton rdoListener;
		private System.Windows.Forms.RadioButton rdoNoListener;
		private System.Windows.Forms.GroupBox groupBox3;
		private System.Windows.Forms.Label lblPeerHost;
		private System.Windows.Forms.RadioButton rdoConnect;
		private System.Windows.Forms.TextBox txtPeerPort;
		private System.Windows.Forms.Label lblPeerPort;
		private System.Windows.Forms.TextBox txtPeerHost;
		private System.Windows.Forms.RadioButton rdoNoConnect;
		private System.Windows.Forms.TextBox txtInstanceName;
		private System.Windows.Forms.Label label5;
		private System.Windows.Forms.TabPage tabConfigure;
		private System.Windows.Forms.Label lblConfigStatus;
		private System.Windows.Forms.ProgressBar prgConfig;
		private System.Windows.Forms.TabPage tabFinish;
		private System.Windows.Forms.Label label1;
	}
}

