namespace Icinga
{
	partial class SetupWizard
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(SetupWizard));
            this.btnBack = new System.Windows.Forms.Button();
            this.btnNext = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.tabFinish = new System.Windows.Forms.TabPage();
            this.lblSetupCompleted = new System.Windows.Forms.Label();
            this.tabConfigure = new System.Windows.Forms.TabPage();
            this.lblConfigStatus = new System.Windows.Forms.Label();
            this.prgConfig = new System.Windows.Forms.ProgressBar();
            this.tabParameters = new System.Windows.Forms.TabPage();
            this.txtParentZone = new System.Windows.Forms.TextBox();
            this.lblParentZone = new System.Windows.Forms.Label();
            this.linkLabelDocs = new System.Windows.Forms.LinkLabel();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.btnEditGlobalZone = new System.Windows.Forms.Button();
            this.btnRemoveGlobalZone = new System.Windows.Forms.Button();
            this.btnAddGlobalZone = new System.Windows.Forms.Button();
            this.lvwGlobalZones = new System.Windows.Forms.ListView();
            this.colGlobalZoneName = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.introduction1 = new System.Windows.Forms.Label();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.chkDisableConf = new System.Windows.Forms.CheckBox();
            this.txtUser = new System.Windows.Forms.TextBox();
            this.chkRunServiceAsThisUser = new System.Windows.Forms.CheckBox();
            this.chkAcceptConfig = new System.Windows.Forms.CheckBox();
            this.chkAcceptCommands = new System.Windows.Forms.CheckBox();
            this.txtTicket = new System.Windows.Forms.TextBox();
            this.lblTicket = new System.Windows.Forms.Label();
            this.txtInstanceName = new System.Windows.Forms.TextBox();
            this.lblInstanceName = new System.Windows.Forms.Label();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.rdoNoListener = new System.Windows.Forms.RadioButton();
            this.txtListenerPort = new System.Windows.Forms.TextBox();
            this.lblListenerPort = new System.Windows.Forms.Label();
            this.rdoListener = new System.Windows.Forms.RadioButton();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.btnEditEndpoint = new System.Windows.Forms.Button();
            this.btnRemoveEndpoint = new System.Windows.Forms.Button();
            this.btnAddEndpoint = new System.Windows.Forms.Button();
            this.lvwEndpoints = new System.Windows.Forms.ListView();
            this.colInstanceName = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.colHost = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.colPort = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.tbcPages = new System.Windows.Forms.TabControl();
            this.tabRetrieveCertificate = new System.Windows.Forms.TabPage();
            this.lblRetrieveCertificate = new System.Windows.Forms.Label();
            this.prgRetrieveCertificate = new System.Windows.Forms.ProgressBar();
            this.tabVerifyCertificate = new System.Windows.Forms.TabPage();
            this.grpX509Fields = new System.Windows.Forms.GroupBox();
            this.txtX509Field = new System.Windows.Forms.TextBox();
            this.lvwX509Fields = new System.Windows.Forms.ListView();
            this.colField = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.colValue = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.txtX509Subject = new System.Windows.Forms.TextBox();
            this.txtX509Issuer = new System.Windows.Forms.TextBox();
            this.lblX509Subject = new System.Windows.Forms.Label();
            this.lblX509Issuer = new System.Windows.Forms.Label();
            this.lblX509Prompt = new System.Windows.Forms.Label();
            this.tabError = new System.Windows.Forms.TabPage();
            this.txtError = new System.Windows.Forms.TextBox();
            this.lblError = new System.Windows.Forms.Label();
            this.picBanner = new System.Windows.Forms.PictureBox();
            this.tabFinish.SuspendLayout();
            this.tabConfigure.SuspendLayout();
            this.tabParameters.SuspendLayout();
            this.groupBox4.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.tbcPages.SuspendLayout();
            this.tabRetrieveCertificate.SuspendLayout();
            this.tabVerifyCertificate.SuspendLayout();
            this.grpX509Fields.SuspendLayout();
            this.tabError.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.picBanner)).BeginInit();
            this.SuspendLayout();
            // 
            // btnBack
            // 
            this.btnBack.Enabled = false;
            this.btnBack.Location = new System.Drawing.Point(376, 564);
            this.btnBack.Name = "btnBack";
            this.btnBack.Size = new System.Drawing.Size(75, 23);
            this.btnBack.TabIndex = 1;
            this.btnBack.Text = "< &Back";
            this.btnBack.UseVisualStyleBackColor = true;
            this.btnBack.Click += new System.EventHandler(this.btnBack_Click);
            // 
            // btnNext
            // 
            this.btnNext.Location = new System.Drawing.Point(457, 564);
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
            this.btnCancel.Location = new System.Drawing.Point(538, 564);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 3;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // tabFinish
            // 
            this.tabFinish.Controls.Add(this.lblSetupCompleted);
            this.tabFinish.Location = new System.Drawing.Point(4, 5);
            this.tabFinish.Name = "tabFinish";
            this.tabFinish.Padding = new System.Windows.Forms.Padding(3);
            this.tabFinish.Size = new System.Drawing.Size(617, 472);
            this.tabFinish.TabIndex = 5;
            this.tabFinish.Text = "Finish";
            this.tabFinish.UseVisualStyleBackColor = true;
            // 
            // lblSetupCompleted
            // 
            this.lblSetupCompleted.AutoSize = true;
            this.lblSetupCompleted.Location = new System.Drawing.Point(34, 35);
            this.lblSetupCompleted.Name = "lblSetupCompleted";
            this.lblSetupCompleted.Size = new System.Drawing.Size(252, 13);
            this.lblSetupCompleted.TabIndex = 0;
            this.lblSetupCompleted.Text = "The Icinga Windows agent was set up successfully.";
            // 
            // tabConfigure
            // 
            this.tabConfigure.Controls.Add(this.lblConfigStatus);
            this.tabConfigure.Controls.Add(this.prgConfig);
            this.tabConfigure.Location = new System.Drawing.Point(4, 5);
            this.tabConfigure.Name = "tabConfigure";
            this.tabConfigure.Padding = new System.Windows.Forms.Padding(3);
            this.tabConfigure.Size = new System.Drawing.Size(617, 472);
            this.tabConfigure.TabIndex = 4;
            this.tabConfigure.Text = "Configure Icinga 2";
            this.tabConfigure.UseVisualStyleBackColor = true;
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
            // prgConfig
            // 
            this.prgConfig.Location = new System.Drawing.Point(184, 223);
            this.prgConfig.Name = "prgConfig";
            this.prgConfig.Size = new System.Drawing.Size(289, 23);
            this.prgConfig.TabIndex = 0;
            // 
            // tabParameters
            // 
            this.tabParameters.Controls.Add(this.txtParentZone);
            this.tabParameters.Controls.Add(this.lblParentZone);
            this.tabParameters.Controls.Add(this.linkLabelDocs);
            this.tabParameters.Controls.Add(this.groupBox4);
            this.tabParameters.Controls.Add(this.introduction1);
            this.tabParameters.Controls.Add(this.groupBox3);
            this.tabParameters.Controls.Add(this.txtTicket);
            this.tabParameters.Controls.Add(this.lblTicket);
            this.tabParameters.Controls.Add(this.txtInstanceName);
            this.tabParameters.Controls.Add(this.lblInstanceName);
            this.tabParameters.Controls.Add(this.groupBox2);
            this.tabParameters.Controls.Add(this.groupBox1);
            this.tabParameters.Location = new System.Drawing.Point(4, 5);
            this.tabParameters.Name = "tabParameters";
            this.tabParameters.Padding = new System.Windows.Forms.Padding(3);
            this.tabParameters.Size = new System.Drawing.Size(617, 472);
            this.tabParameters.TabIndex = 3;
            this.tabParameters.Text = "Agent Parameters";
            this.tabParameters.UseVisualStyleBackColor = true;
            // 
            // txtParentZone
            // 
            this.txtParentZone.Location = new System.Drawing.Point(528, 56);
            this.txtParentZone.Name = "txtParentZone";
            this.txtParentZone.Size = new System.Drawing.Size(73, 20);
            this.txtParentZone.TabIndex = 12;
            this.txtParentZone.Text = "master";
            // 
            // lblParentZone
            // 
            this.lblParentZone.AutoSize = true;
            this.lblParentZone.Location = new System.Drawing.Point(525, 30);
            this.lblParentZone.Name = "lblParentZone";
            this.lblParentZone.Size = new System.Drawing.Size(69, 13);
            this.lblParentZone.TabIndex = 11;
            this.lblParentZone.Text = "Parent Zone:";
            // 
            // linkLabelDocs
            // 
            this.linkLabelDocs.AutoSize = true;
            this.linkLabelDocs.LinkColor = System.Drawing.Color.FromArgb(((int)(((byte)(0)))), ((int)(((byte)(149)))), ((int)(((byte)(191)))));
            this.linkLabelDocs.Location = new System.Drawing.Point(525, 3);
            this.linkLabelDocs.Name = "linkLabelDocs";
            this.linkLabelDocs.Size = new System.Drawing.Size(79, 13);
            this.linkLabelDocs.TabIndex = 10;
            this.linkLabelDocs.TabStop = true;
            this.linkLabelDocs.Text = "Documentation";
            this.linkLabelDocs.VisitedLinkColor = System.Drawing.Color.FromArgb(((int)(((byte)(0)))), ((int)(((byte)(149)))), ((int)(((byte)(191)))));
            this.linkLabelDocs.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelDocs_LinkClicked);
            // 
            // groupBox4
            // 
            this.groupBox4.Controls.Add(this.btnEditGlobalZone);
            this.groupBox4.Controls.Add(this.btnRemoveGlobalZone);
            this.groupBox4.Controls.Add(this.btnAddGlobalZone);
            this.groupBox4.Controls.Add(this.lvwGlobalZones);
            this.groupBox4.Location = new System.Drawing.Point(8, 210);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Size = new System.Drawing.Size(601, 110);
            this.groupBox4.TabIndex = 9;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "Global Zones";
            // 
            // btnEditGlobalZone
            // 
            this.btnEditGlobalZone.Enabled = false;
            this.btnEditGlobalZone.Location = new System.Drawing.Point(520, 48);
            this.btnEditGlobalZone.Name = "btnEditGlobalZone";
            this.btnEditGlobalZone.Size = new System.Drawing.Size(75, 23);
            this.btnEditGlobalZone.TabIndex = 7;
            this.btnEditGlobalZone.Text = "Edit";
            this.btnEditGlobalZone.UseVisualStyleBackColor = true;
            this.btnEditGlobalZone.Click += new System.EventHandler(this.btnEditGlobalZone_Click);
            // 
            // btnRemoveGlobalZone
            // 
            this.btnRemoveGlobalZone.Enabled = false;
            this.btnRemoveGlobalZone.Location = new System.Drawing.Point(520, 77);
            this.btnRemoveGlobalZone.Name = "btnRemoveGlobalZone";
            this.btnRemoveGlobalZone.Size = new System.Drawing.Size(75, 23);
            this.btnRemoveGlobalZone.TabIndex = 6;
            this.btnRemoveGlobalZone.Text = "Remove";
            this.btnRemoveGlobalZone.UseVisualStyleBackColor = true;
            this.btnRemoveGlobalZone.Click += new System.EventHandler(this.btnRemoveGlobalZone_Click);
            // 
            // btnAddGlobalZone
            // 
            this.btnAddGlobalZone.Location = new System.Drawing.Point(520, 19);
            this.btnAddGlobalZone.Name = "btnAddGlobalZone";
            this.btnAddGlobalZone.Size = new System.Drawing.Size(75, 23);
            this.btnAddGlobalZone.TabIndex = 5;
            this.btnAddGlobalZone.Text = "Add";
            this.btnAddGlobalZone.UseVisualStyleBackColor = true;
            this.btnAddGlobalZone.Click += new System.EventHandler(this.btnAddGlobalZone_Click);
            // 
            // lvwGlobalZones
            // 
            this.lvwGlobalZones.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colGlobalZoneName});
            this.lvwGlobalZones.FullRowSelect = true;
            this.lvwGlobalZones.HideSelection = false;
            this.lvwGlobalZones.Location = new System.Drawing.Point(6, 19);
            this.lvwGlobalZones.Name = "lvwGlobalZones";
            this.lvwGlobalZones.Size = new System.Drawing.Size(500, 81);
            this.lvwGlobalZones.TabIndex = 4;
            this.lvwGlobalZones.UseCompatibleStateImageBehavior = false;
            this.lvwGlobalZones.View = System.Windows.Forms.View.Details;
            this.lvwGlobalZones.SelectedIndexChanged += new System.EventHandler(this.lvwGlobalZones_SelectedIndexChanged);
            // 
            // colGlobalZoneName
            // 
            this.colGlobalZoneName.Text = "Zone Name";
            this.colGlobalZoneName.Width = 496;
            // 
            // introduction1
            // 
            this.introduction1.AutoSize = true;
            this.introduction1.Location = new System.Drawing.Point(11, 3);
            this.introduction1.Name = "introduction1";
            this.introduction1.Size = new System.Drawing.Size(262, 13);
            this.introduction1.TabIndex = 6;
            this.introduction1.Text = "Welcome to the Icinga Windows Agent Setup Wizard!";
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.chkDisableConf);
            this.groupBox3.Controls.Add(this.txtUser);
            this.groupBox3.Controls.Add(this.chkRunServiceAsThisUser);
            this.groupBox3.Controls.Add(this.chkAcceptConfig);
            this.groupBox3.Controls.Add(this.chkAcceptCommands);
            this.groupBox3.Location = new System.Drawing.Point(308, 326);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(301, 140);
            this.groupBox3.TabIndex = 5;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Advanced Options";
            // 
            // chkDisableConf
            // 
            this.chkDisableConf.AutoSize = true;
            this.chkDisableConf.Checked = true;
            this.chkDisableConf.CheckState = System.Windows.Forms.CheckState.Checked;
            this.chkDisableConf.Location = new System.Drawing.Point(9, 114);
            this.chkDisableConf.Name = "chkDisableConf";
            this.chkDisableConf.Size = new System.Drawing.Size(211, 17);
            this.chkDisableConf.TabIndex = 9;
            this.chkDisableConf.Text = "Disable including local \'conf.d\' directory";
            this.chkDisableConf.UseVisualStyleBackColor = true;
            // 
            // txtUser
            // 
            this.txtUser.Enabled = false;
            this.txtUser.Location = new System.Drawing.Point(28, 88);
            this.txtUser.Name = "txtUser";
            this.txtUser.Size = new System.Drawing.Size(178, 20);
            this.txtUser.TabIndex = 8;
            this.txtUser.Text = "NT AUTHORITY\\NetworkService";
            // 
            // chkRunServiceAsThisUser
            // 
            this.chkRunServiceAsThisUser.AutoSize = true;
            this.chkRunServiceAsThisUser.Location = new System.Drawing.Point(9, 65);
            this.chkRunServiceAsThisUser.Name = "chkRunServiceAsThisUser";
            this.chkRunServiceAsThisUser.Size = new System.Drawing.Size(183, 17);
            this.chkRunServiceAsThisUser.TabIndex = 7;
            this.chkRunServiceAsThisUser.Text = "Run Icinga 2 service as this user:";
            this.chkRunServiceAsThisUser.UseVisualStyleBackColor = true;
            this.chkRunServiceAsThisUser.CheckedChanged += new System.EventHandler(this.chkRunServiceAsThisUser_CheckedChanged);
            // 
            // chkAcceptConfig
            // 
            this.chkAcceptConfig.AutoSize = true;
            this.chkAcceptConfig.Location = new System.Drawing.Point(9, 42);
            this.chkAcceptConfig.Name = "chkAcceptConfig";
            this.chkAcceptConfig.Size = new System.Drawing.Size(284, 17);
            this.chkAcceptConfig.TabIndex = 1;
            this.chkAcceptConfig.Text = "Accept config updates from master/satellite instance(s)";
            this.chkAcceptConfig.UseVisualStyleBackColor = true;
            // 
            // chkAcceptCommands
            // 
            this.chkAcceptCommands.AutoSize = true;
            this.chkAcceptCommands.Location = new System.Drawing.Point(9, 19);
            this.chkAcceptCommands.Name = "chkAcceptCommands";
            this.chkAcceptCommands.Size = new System.Drawing.Size(265, 17);
            this.chkAcceptCommands.TabIndex = 0;
            this.chkAcceptCommands.Text = "Accept commands from master/satellite instance(s)";
            this.chkAcceptCommands.UseVisualStyleBackColor = true;
            // 
            // txtTicket
            // 
            this.txtTicket.Location = new System.Drawing.Point(164, 56);
            this.txtTicket.Name = "txtTicket";
            this.txtTicket.Size = new System.Drawing.Size(350, 20);
            this.txtTicket.TabIndex = 1;
            // 
            // lblTicket
            // 
            this.lblTicket.AutoSize = true;
            this.lblTicket.Location = new System.Drawing.Point(9, 59);
            this.lblTicket.Name = "lblTicket";
            this.lblTicket.Size = new System.Drawing.Size(149, 13);
            this.lblTicket.TabIndex = 4;
            this.lblTicket.Text = "CSR Signing Ticket (optional):";
            // 
            // txtInstanceName
            // 
            this.txtInstanceName.Location = new System.Drawing.Point(164, 27);
            this.txtInstanceName.Name = "txtInstanceName";
            this.txtInstanceName.Size = new System.Drawing.Size(350, 20);
            this.txtInstanceName.TabIndex = 0;
            // 
            // lblInstanceName
            // 
            this.lblInstanceName.AutoSize = true;
            this.lblInstanceName.Location = new System.Drawing.Point(11, 30);
            this.lblInstanceName.Name = "lblInstanceName";
            this.lblInstanceName.Size = new System.Drawing.Size(121, 13);
            this.lblInstanceName.TabIndex = 3;
            this.lblInstanceName.Text = "Instance Name (FQDN):";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.rdoNoListener);
            this.groupBox2.Controls.Add(this.txtListenerPort);
            this.groupBox2.Controls.Add(this.lblListenerPort);
            this.groupBox2.Controls.Add(this.rdoListener);
            this.groupBox2.Location = new System.Drawing.Point(8, 326);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(298, 140);
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
            this.rdoNoListener.TabIndex = 9;
            this.rdoNoListener.TabStop = true;
            this.rdoNoListener.Text = "Do not listen for connections.";
            this.rdoNoListener.UseVisualStyleBackColor = true;
            this.rdoNoListener.CheckedChanged += new System.EventHandler(this.RadioListener_CheckedChanged);
            // 
            // txtListenerPort
            // 
            this.txtListenerPort.Enabled = false;
            this.txtListenerPort.Location = new System.Drawing.Point(66, 47);
            this.txtListenerPort.Name = "txtListenerPort";
            this.txtListenerPort.Size = new System.Drawing.Size(84, 20);
            this.txtListenerPort.TabIndex = 8;
            this.txtListenerPort.Text = "5665";
            // 
            // lblListenerPort
            // 
            this.lblListenerPort.AutoSize = true;
            this.lblListenerPort.Location = new System.Drawing.Point(31, 51);
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
            this.rdoListener.Size = new System.Drawing.Size(283, 17);
            this.rdoListener.TabIndex = 7;
            this.rdoListener.Text = "Listen for connections from master/satellite instance(s):";
            this.rdoListener.UseVisualStyleBackColor = true;
            this.rdoListener.CheckedChanged += new System.EventHandler(this.RadioListener_CheckedChanged);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.btnEditEndpoint);
            this.groupBox1.Controls.Add(this.btnRemoveEndpoint);
            this.groupBox1.Controls.Add(this.btnAddEndpoint);
            this.groupBox1.Controls.Add(this.lvwEndpoints);
            this.groupBox1.Location = new System.Drawing.Point(8, 94);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(601, 110);
            this.groupBox1.TabIndex = 1;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Parent master/satellite instance(s) for this agent";
            // 
            // btnEditEndpoint
            // 
            this.btnEditEndpoint.Enabled = false;
            this.btnEditEndpoint.Location = new System.Drawing.Point(520, 48);
            this.btnEditEndpoint.Name = "btnEditEndpoint";
            this.btnEditEndpoint.Size = new System.Drawing.Size(75, 23);
            this.btnEditEndpoint.TabIndex = 7;
            this.btnEditEndpoint.Text = "Edit";
            this.btnEditEndpoint.UseVisualStyleBackColor = true;
            this.btnEditEndpoint.Click += new System.EventHandler(this.btnEditEndpoint_Click);
            // 
            // btnRemoveEndpoint
            // 
            this.btnRemoveEndpoint.Enabled = false;
            this.btnRemoveEndpoint.Location = new System.Drawing.Point(520, 77);
            this.btnRemoveEndpoint.Name = "btnRemoveEndpoint";
            this.btnRemoveEndpoint.Size = new System.Drawing.Size(75, 23);
            this.btnRemoveEndpoint.TabIndex = 6;
            this.btnRemoveEndpoint.Text = "Remove";
            this.btnRemoveEndpoint.UseVisualStyleBackColor = true;
            this.btnRemoveEndpoint.Click += new System.EventHandler(this.btnRemoveEndpoint_Click);
            // 
            // btnAddEndpoint
            // 
            this.btnAddEndpoint.Location = new System.Drawing.Point(520, 19);
            this.btnAddEndpoint.Name = "btnAddEndpoint";
            this.btnAddEndpoint.Size = new System.Drawing.Size(75, 23);
            this.btnAddEndpoint.TabIndex = 5;
            this.btnAddEndpoint.Text = "Add";
            this.btnAddEndpoint.UseVisualStyleBackColor = true;
            this.btnAddEndpoint.Click += new System.EventHandler(this.btnAddEndpoint_Click);
            // 
            // lvwEndpoints
            // 
            this.lvwEndpoints.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colInstanceName,
            this.colHost,
            this.colPort});
            this.lvwEndpoints.FullRowSelect = true;
            this.lvwEndpoints.HideSelection = false;
            this.lvwEndpoints.Location = new System.Drawing.Point(6, 19);
            this.lvwEndpoints.Name = "lvwEndpoints";
            this.lvwEndpoints.Size = new System.Drawing.Size(500, 81);
            this.lvwEndpoints.TabIndex = 4;
            this.lvwEndpoints.UseCompatibleStateImageBehavior = false;
            this.lvwEndpoints.View = System.Windows.Forms.View.Details;
            this.lvwEndpoints.SelectedIndexChanged += new System.EventHandler(this.lvwEndpoints_SelectedIndexChanged);
            // 
            // colInstanceName
            // 
            this.colInstanceName.Text = "Instance Name";
            this.colInstanceName.Width = 200;
            // 
            // colHost
            // 
            this.colHost.Text = "Host";
            this.colHost.Width = 200;
            // 
            // colPort
            // 
            this.colPort.Text = "Port";
            this.colPort.Width = 80;
            // 
            // tbcPages
            // 
            this.tbcPages.Appearance = System.Windows.Forms.TabAppearance.FlatButtons;
            this.tbcPages.Controls.Add(this.tabParameters);
            this.tbcPages.Controls.Add(this.tabRetrieveCertificate);
            this.tbcPages.Controls.Add(this.tabVerifyCertificate);
            this.tbcPages.Controls.Add(this.tabConfigure);
            this.tbcPages.Controls.Add(this.tabFinish);
            this.tbcPages.Controls.Add(this.tabError);
            this.tbcPages.ItemSize = new System.Drawing.Size(0, 1);
            this.tbcPages.Location = new System.Drawing.Point(0, 80);
            this.tbcPages.Margin = new System.Windows.Forms.Padding(0);
            this.tbcPages.Name = "tbcPages";
            this.tbcPages.SelectedIndex = 0;
            this.tbcPages.Size = new System.Drawing.Size(625, 481);
            this.tbcPages.SizeMode = System.Windows.Forms.TabSizeMode.Fixed;
            this.tbcPages.TabIndex = 0;
            this.tbcPages.SelectedIndexChanged += new System.EventHandler(this.tbcPages_SelectedIndexChanged);
            // 
            // tabRetrieveCertificate
            // 
            this.tabRetrieveCertificate.Controls.Add(this.lblRetrieveCertificate);
            this.tabRetrieveCertificate.Controls.Add(this.prgRetrieveCertificate);
            this.tabRetrieveCertificate.Location = new System.Drawing.Point(4, 5);
            this.tabRetrieveCertificate.Name = "tabRetrieveCertificate";
            this.tabRetrieveCertificate.Padding = new System.Windows.Forms.Padding(3);
            this.tabRetrieveCertificate.Size = new System.Drawing.Size(617, 472);
            this.tabRetrieveCertificate.TabIndex = 7;
            this.tabRetrieveCertificate.Text = "Checking Certificate";
            this.tabRetrieveCertificate.UseVisualStyleBackColor = true;
            // 
            // lblRetrieveCertificate
            // 
            this.lblRetrieveCertificate.AutoSize = true;
            this.lblRetrieveCertificate.Location = new System.Drawing.Point(164, 229);
            this.lblRetrieveCertificate.Name = "lblRetrieveCertificate";
            this.lblRetrieveCertificate.Size = new System.Drawing.Size(110, 13);
            this.lblRetrieveCertificate.TabIndex = 3;
            this.lblRetrieveCertificate.Text = "Checking certificate...";
            // 
            // prgRetrieveCertificate
            // 
            this.prgRetrieveCertificate.Location = new System.Drawing.Point(164, 248);
            this.prgRetrieveCertificate.Name = "prgRetrieveCertificate";
            this.prgRetrieveCertificate.Size = new System.Drawing.Size(289, 23);
            this.prgRetrieveCertificate.TabIndex = 2;
            // 
            // tabVerifyCertificate
            // 
            this.tabVerifyCertificate.Controls.Add(this.grpX509Fields);
            this.tabVerifyCertificate.Controls.Add(this.txtX509Subject);
            this.tabVerifyCertificate.Controls.Add(this.txtX509Issuer);
            this.tabVerifyCertificate.Controls.Add(this.lblX509Subject);
            this.tabVerifyCertificate.Controls.Add(this.lblX509Issuer);
            this.tabVerifyCertificate.Controls.Add(this.lblX509Prompt);
            this.tabVerifyCertificate.Location = new System.Drawing.Point(4, 5);
            this.tabVerifyCertificate.Name = "tabVerifyCertificate";
            this.tabVerifyCertificate.Padding = new System.Windows.Forms.Padding(3);
            this.tabVerifyCertificate.Size = new System.Drawing.Size(617, 472);
            this.tabVerifyCertificate.TabIndex = 6;
            this.tabVerifyCertificate.Text = "Verify Certificate";
            this.tabVerifyCertificate.UseVisualStyleBackColor = true;
            // 
            // grpX509Fields
            // 
            this.grpX509Fields.Controls.Add(this.txtX509Field);
            this.grpX509Fields.Controls.Add(this.lvwX509Fields);
            this.grpX509Fields.Location = new System.Drawing.Point(11, 115);
            this.grpX509Fields.Name = "grpX509Fields";
            this.grpX509Fields.Size = new System.Drawing.Size(598, 369);
            this.grpX509Fields.TabIndex = 8;
            this.grpX509Fields.TabStop = false;
            this.grpX509Fields.Text = "X509 Fields";
            // 
            // txtX509Field
            // 
            this.txtX509Field.Font = new System.Drawing.Font("Courier New", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.txtX509Field.Location = new System.Drawing.Point(6, 197);
            this.txtX509Field.Multiline = true;
            this.txtX509Field.Name = "txtX509Field";
            this.txtX509Field.ReadOnly = true;
            this.txtX509Field.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.txtX509Field.Size = new System.Drawing.Size(586, 166);
            this.txtX509Field.TabIndex = 9;
            // 
            // lvwX509Fields
            // 
            this.lvwX509Fields.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colField,
            this.colValue});
            this.lvwX509Fields.HideSelection = false;
            this.lvwX509Fields.Location = new System.Drawing.Point(6, 19);
            this.lvwX509Fields.Name = "lvwX509Fields";
            this.lvwX509Fields.Size = new System.Drawing.Size(586, 172);
            this.lvwX509Fields.TabIndex = 8;
            this.lvwX509Fields.UseCompatibleStateImageBehavior = false;
            this.lvwX509Fields.View = System.Windows.Forms.View.Details;
            this.lvwX509Fields.SelectedIndexChanged += new System.EventHandler(this.lvwX509Fields_SelectedIndexChanged);
            // 
            // colField
            // 
            this.colField.Text = "Field";
            this.colField.Width = 200;
            // 
            // colValue
            // 
            this.colValue.Text = "Value";
            this.colValue.Width = 350;
            // 
            // txtX509Subject
            // 
            this.txtX509Subject.Location = new System.Drawing.Point(71, 73);
            this.txtX509Subject.Name = "txtX509Subject";
            this.txtX509Subject.ReadOnly = true;
            this.txtX509Subject.Size = new System.Drawing.Size(532, 20);
            this.txtX509Subject.TabIndex = 4;
            // 
            // txtX509Issuer
            // 
            this.txtX509Issuer.Location = new System.Drawing.Point(71, 47);
            this.txtX509Issuer.Name = "txtX509Issuer";
            this.txtX509Issuer.ReadOnly = true;
            this.txtX509Issuer.Size = new System.Drawing.Size(532, 20);
            this.txtX509Issuer.TabIndex = 3;
            // 
            // lblX509Subject
            // 
            this.lblX509Subject.AutoSize = true;
            this.lblX509Subject.Location = new System.Drawing.Point(8, 77);
            this.lblX509Subject.Name = "lblX509Subject";
            this.lblX509Subject.Size = new System.Drawing.Size(46, 13);
            this.lblX509Subject.TabIndex = 2;
            this.lblX509Subject.Text = "Subject:";
            // 
            // lblX509Issuer
            // 
            this.lblX509Issuer.AutoSize = true;
            this.lblX509Issuer.Location = new System.Drawing.Point(8, 50);
            this.lblX509Issuer.Name = "lblX509Issuer";
            this.lblX509Issuer.Size = new System.Drawing.Size(38, 13);
            this.lblX509Issuer.TabIndex = 1;
            this.lblX509Issuer.Text = "Issuer:";
            // 
            // lblX509Prompt
            // 
            this.lblX509Prompt.AutoSize = true;
            this.lblX509Prompt.Location = new System.Drawing.Point(8, 15);
            this.lblX509Prompt.Name = "lblX509Prompt";
            this.lblX509Prompt.Size = new System.Drawing.Size(241, 13);
            this.lblX509Prompt.TabIndex = 0;
            this.lblX509Prompt.Text = "Please verify the master/satellite\'s SSL certificate:";
            // 
            // tabError
            // 
            this.tabError.Controls.Add(this.txtError);
            this.tabError.Controls.Add(this.lblError);
            this.tabError.Location = new System.Drawing.Point(4, 5);
            this.tabError.Name = "tabError";
            this.tabError.Padding = new System.Windows.Forms.Padding(3);
            this.tabError.Size = new System.Drawing.Size(617, 472);
            this.tabError.TabIndex = 8;
            this.tabError.Text = "Error";
            this.tabError.UseVisualStyleBackColor = true;
            // 
            // txtError
            // 
            this.txtError.Font = new System.Drawing.Font("Courier New", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.txtError.Location = new System.Drawing.Point(11, 38);
            this.txtError.Multiline = true;
            this.txtError.Name = "txtError";
            this.txtError.ReadOnly = true;
            this.txtError.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.txtError.Size = new System.Drawing.Size(598, 397);
            this.txtError.TabIndex = 1;
            // 
            // lblError
            // 
            this.lblError.AutoSize = true;
            this.lblError.Location = new System.Drawing.Point(8, 12);
            this.lblError.Name = "lblError";
            this.lblError.Size = new System.Drawing.Size(209, 13);
            this.lblError.TabIndex = 0;
            this.lblError.Text = "An error occurred while setting up Icinga 2:";
            // 
            // picBanner
            // 
            this.picBanner.Image = ((System.Drawing.Image)(resources.GetObject("picBanner.Image")));
            this.picBanner.Location = new System.Drawing.Point(0, 0);
            this.picBanner.Name = "picBanner";
            this.picBanner.Size = new System.Drawing.Size(625, 77);
            this.picBanner.TabIndex = 1;
            this.picBanner.TabStop = false;
            // 
            // SetupWizard
            // 
            this.AcceptButton = this.btnNext;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(625, 599);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnNext);
            this.Controls.Add(this.btnBack);
            this.Controls.Add(this.picBanner);
            this.Controls.Add(this.tbcPages);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "SetupWizard";
            this.Text = "Icinga Windows Agent Setup Wizard";
            this.Load += new System.EventHandler(this.SetupWizard_Load);
            this.tabFinish.ResumeLayout(false);
            this.tabFinish.PerformLayout();
            this.tabConfigure.ResumeLayout(false);
            this.tabConfigure.PerformLayout();
            this.tabParameters.ResumeLayout(false);
            this.tabParameters.PerformLayout();
            this.groupBox4.ResumeLayout(false);
            this.groupBox3.ResumeLayout(false);
            this.groupBox3.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.groupBox1.ResumeLayout(false);
            this.tbcPages.ResumeLayout(false);
            this.tabRetrieveCertificate.ResumeLayout(false);
            this.tabRetrieveCertificate.PerformLayout();
            this.tabVerifyCertificate.ResumeLayout(false);
            this.tabVerifyCertificate.PerformLayout();
            this.grpX509Fields.ResumeLayout(false);
            this.grpX509Fields.PerformLayout();
            this.tabError.ResumeLayout(false);
            this.tabError.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.picBanner)).EndInit();
            this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.PictureBox picBanner;
		private System.Windows.Forms.Button btnBack;
		private System.Windows.Forms.Button btnNext;
		private System.Windows.Forms.Button btnCancel;
		private System.Windows.Forms.TabPage tabFinish;
		private System.Windows.Forms.Label lblSetupCompleted;
		private System.Windows.Forms.TabPage tabConfigure;
		private System.Windows.Forms.Label lblConfigStatus;
		private System.Windows.Forms.ProgressBar prgConfig;
		private System.Windows.Forms.TabPage tabParameters;
		private System.Windows.Forms.TextBox txtInstanceName;
		private System.Windows.Forms.Label lblInstanceName;
		private System.Windows.Forms.GroupBox groupBox2;
		private System.Windows.Forms.RadioButton rdoNoListener;
		private System.Windows.Forms.TextBox txtListenerPort;
		private System.Windows.Forms.Label lblListenerPort;
		private System.Windows.Forms.RadioButton rdoListener;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.Button btnRemoveEndpoint;
		private System.Windows.Forms.Button btnAddEndpoint;
		private System.Windows.Forms.ListView lvwEndpoints;
		private System.Windows.Forms.ColumnHeader colHost;
		private System.Windows.Forms.ColumnHeader colPort;
		private System.Windows.Forms.TabControl tbcPages;
		private System.Windows.Forms.TabPage tabVerifyCertificate;
		private System.Windows.Forms.Label lblX509Prompt;
		private System.Windows.Forms.TextBox txtX509Subject;
		private System.Windows.Forms.TextBox txtX509Issuer;
		private System.Windows.Forms.Label lblX509Subject;
		private System.Windows.Forms.Label lblX509Issuer;
		private System.Windows.Forms.GroupBox grpX509Fields;
		private System.Windows.Forms.ListView lvwX509Fields;
		private System.Windows.Forms.ColumnHeader colField;
		private System.Windows.Forms.ColumnHeader colValue;
		private System.Windows.Forms.TextBox txtX509Field;
		private System.Windows.Forms.TabPage tabRetrieveCertificate;
		private System.Windows.Forms.Label lblRetrieveCertificate;
		private System.Windows.Forms.ProgressBar prgRetrieveCertificate;
		private System.Windows.Forms.TabPage tabError;
		private System.Windows.Forms.TextBox txtError;
		private System.Windows.Forms.Label lblError;
		private System.Windows.Forms.TextBox txtTicket;
		private System.Windows.Forms.Label lblTicket;
		private System.Windows.Forms.ColumnHeader colInstanceName;
		private System.Windows.Forms.GroupBox groupBox3;
		private System.Windows.Forms.CheckBox chkAcceptConfig;
		private System.Windows.Forms.CheckBox chkAcceptCommands;
		private System.Windows.Forms.TextBox txtUser;
		private System.Windows.Forms.CheckBox chkRunServiceAsThisUser;
		private System.Windows.Forms.Button btnEditEndpoint;
		private System.Windows.Forms.Label introduction1;
		private System.Windows.Forms.GroupBox groupBox4;
		private System.Windows.Forms.Button btnEditGlobalZone;
		private System.Windows.Forms.Button btnRemoveGlobalZone;
		private System.Windows.Forms.Button btnAddGlobalZone;
		private System.Windows.Forms.ListView lvwGlobalZones;
		private System.Windows.Forms.ColumnHeader colGlobalZoneName;
		private System.Windows.Forms.CheckBox chkDisableConf;
		private System.Windows.Forms.LinkLabel linkLabelDocs;
        private System.Windows.Forms.TextBox txtParentZone;
        private System.Windows.Forms.Label lblParentZone;
    }
}

