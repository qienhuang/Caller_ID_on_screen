namespace cider
{
    partial class MainForm
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
            this.notifyIcon1 = new System.Windows.Forms.NotifyIcon(this.components);
            this.contextMenuStrip1 = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.openMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.showMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.exitMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.aboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.lbNumber = new System.Windows.Forms.Label();
            this.lbName = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.tbCustomer = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.tbAddress = new System.Windows.Forms.TextBox();
            this.label4 = new System.Windows.Forms.Label();
            this.tbLastInvoice = new System.Windows.Forms.TextBox();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.tbCity = new System.Windows.Forms.TextBox();
            this.tbState = new System.Windows.Forms.TextBox();
            this.btClose = new System.Windows.Forms.Button();
            this.lbTime = new System.Windows.Forms.Label();
            this.btnLog = new System.Windows.Forms.Button();
            this.listView1 = new System.Windows.Forms.ListView();
            this.colIndex = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.colDate = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.colTime = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.colPhoneNumber = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.colCallerID = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.colCustomer = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.colAddress = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.colCity = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.colState = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.btPrevRecord = new System.Windows.Forms.Button();
            this.btNextRecord = new System.Windows.Forms.Button();
            this.lbUdpTimeout = new System.Windows.Forms.Label();
            this.contextMenuStrip1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // notifyIcon1
            // 
            this.notifyIcon1.ContextMenuStrip = this.contextMenuStrip1;
            this.notifyIcon1.Icon = ((System.Drawing.Icon)(resources.GetObject("notifyIcon1.Icon")));
            this.notifyIcon1.Text = "notifyIcon1";
            this.notifyIcon1.Visible = true;
            this.notifyIcon1.MouseClick += new System.Windows.Forms.MouseEventHandler(this.notifyIcon1_MouseClick);
            // 
            // contextMenuStrip1
            // 
            this.contextMenuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.openMenuItem,
            this.showMenuItem,
            this.exitMenuItem,
            this.aboutToolStripMenuItem});
            this.contextMenuStrip1.Name = "contextMenuStrip1";
            this.contextMenuStrip1.Size = new System.Drawing.Size(150, 92);
            // 
            // openMenuItem
            // 
            this.openMenuItem.Name = "openMenuItem";
            this.openMenuItem.Size = new System.Drawing.Size(149, 22);
            this.openMenuItem.Text = "&Open call logs";
            this.openMenuItem.Click += new System.EventHandler(this.openMenuItem_Click);
            // 
            // showMenuItem
            // 
            this.showMenuItem.Name = "showMenuItem";
            this.showMenuItem.Size = new System.Drawing.Size(149, 22);
            this.showMenuItem.Text = "&Show";
            this.showMenuItem.Click += new System.EventHandler(this.showMenuItem_Click);
            // 
            // exitMenuItem
            // 
            this.exitMenuItem.Name = "exitMenuItem";
            this.exitMenuItem.Size = new System.Drawing.Size(149, 22);
            this.exitMenuItem.Text = "&Exit";
            this.exitMenuItem.Click += new System.EventHandler(this.exitMenuItem_Click);
            // 
            // aboutToolStripMenuItem
            // 
            this.aboutToolStripMenuItem.Name = "aboutToolStripMenuItem";
            this.aboutToolStripMenuItem.Size = new System.Drawing.Size(149, 22);
            this.aboutToolStripMenuItem.Text = "&About";
            this.aboutToolStripMenuItem.Click += new System.EventHandler(this.aboutToolStripMenuItem_Click);
            // 
            // lbNumber
            // 
            this.lbNumber.AutoSize = true;
            this.lbNumber.BackColor = System.Drawing.Color.Transparent;
            this.lbNumber.Font = new System.Drawing.Font("Tahoma", 16F);
            this.lbNumber.ForeColor = System.Drawing.SystemColors.ControlText;
            this.lbNumber.Location = new System.Drawing.Point(56, 9);
            this.lbNumber.Name = "lbNumber";
            this.lbNumber.Size = new System.Drawing.Size(148, 27);
            this.lbNumber.TabIndex = 1;
            this.lbNumber.Text = "347-000-0000";
            // 
            // lbName
            // 
            this.lbName.AutoSize = true;
            this.lbName.BackColor = System.Drawing.Color.Transparent;
            this.lbName.Font = new System.Drawing.Font("Tahoma", 16F);
            this.lbName.ForeColor = System.Drawing.SystemColors.ControlText;
            this.lbName.Location = new System.Drawing.Point(56, 34);
            this.lbName.Name = "lbName";
            this.lbName.Size = new System.Drawing.Size(152, 27);
            this.lbName.TabIndex = 2;
            this.lbName.Text = "KEVIN HUANG";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label2.Location = new System.Drawing.Point(275, 8);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(54, 13);
            this.label2.TabIndex = 3;
            this.label2.Text = "Customer:";
            // 
            // tbCustomer
            // 
            this.tbCustomer.BackColor = System.Drawing.Color.White;
            this.tbCustomer.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.tbCustomer.ForeColor = System.Drawing.Color.Black;
            this.tbCustomer.Location = new System.Drawing.Point(335, 9);
            this.tbCustomer.Name = "tbCustomer";
            this.tbCustomer.ReadOnly = true;
            this.tbCustomer.Size = new System.Drawing.Size(218, 13);
            this.tbCustomer.TabIndex = 4;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label3.Location = new System.Drawing.Point(261, 80);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(68, 13);
            this.label3.TabIndex = 5;
            this.label3.Text = "Last Invoice:";
            // 
            // tbAddress
            // 
            this.tbAddress.BackColor = System.Drawing.Color.White;
            this.tbAddress.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.tbAddress.Location = new System.Drawing.Point(335, 33);
            this.tbAddress.Name = "tbAddress";
            this.tbAddress.ReadOnly = true;
            this.tbAddress.Size = new System.Drawing.Size(218, 13);
            this.tbAddress.TabIndex = 6;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label4.Location = new System.Drawing.Point(281, 33);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(48, 13);
            this.label4.TabIndex = 7;
            this.label4.Text = "Address:";
            // 
            // tbLastInvoice
            // 
            this.tbLastInvoice.BackColor = System.Drawing.Color.White;
            this.tbLastInvoice.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.tbLastInvoice.Location = new System.Drawing.Point(335, 80);
            this.tbLastInvoice.Name = "tbLastInvoice";
            this.tbLastInvoice.ReadOnly = true;
            this.tbLastInvoice.Size = new System.Drawing.Size(154, 13);
            this.tbLastInvoice.TabIndex = 8;
            // 
            // pictureBox1
            // 
            this.pictureBox1.Image = global::cider.Properties.Resources.connect_10503;
            this.pictureBox1.Location = new System.Drawing.Point(13, 16);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(37, 42);
            this.pictureBox1.TabIndex = 9;
            this.pictureBox1.TabStop = false;
            // 
            // tbCity
            // 
            this.tbCity.BackColor = System.Drawing.Color.White;
            this.tbCity.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.tbCity.Location = new System.Drawing.Point(335, 56);
            this.tbCity.Name = "tbCity";
            this.tbCity.Size = new System.Drawing.Size(105, 13);
            this.tbCity.TabIndex = 10;
            // 
            // tbState
            // 
            this.tbState.BackColor = System.Drawing.Color.White;
            this.tbState.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.tbState.Location = new System.Drawing.Point(446, 56);
            this.tbState.Name = "tbState";
            this.tbState.Size = new System.Drawing.Size(43, 13);
            this.tbState.TabIndex = 11;
            // 
            // btClose
            // 
            this.btClose.Location = new System.Drawing.Point(446, 109);
            this.btClose.Name = "btClose";
            this.btClose.Size = new System.Drawing.Size(105, 22);
            this.btClose.TabIndex = 12;
            this.btClose.Text = "&Close";
            this.btClose.UseVisualStyleBackColor = true;
            this.btClose.Click += new System.EventHandler(this.btClose_Click);
            // 
            // lbTime
            // 
            this.lbTime.AutoSize = true;
            this.lbTime.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.lbTime.Location = new System.Drawing.Point(58, 64);
            this.lbTime.Name = "lbTime";
            this.lbTime.Size = new System.Drawing.Size(62, 13);
            this.lbTime.TabIndex = 15;
            this.lbTime.Text = "Label_Time";
            // 
            // btnLog
            // 
            this.btnLog.Location = new System.Drawing.Point(335, 109);
            this.btnLog.Name = "btnLog";
            this.btnLog.Size = new System.Drawing.Size(105, 22);
            this.btnLog.TabIndex = 16;
            this.btnLog.Text = "Show &Log >>>";
            this.btnLog.UseVisualStyleBackColor = true;
            this.btnLog.Click += new System.EventHandler(this.btnLog_Click);
            // 
            // listView1
            // 
            this.listView1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.listView1.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colIndex,
            this.colDate,
            this.colTime,
            this.colPhoneNumber,
            this.colCallerID,
            this.colCustomer,
            this.colAddress,
            this.colCity,
            this.colState});
            this.listView1.FullRowSelect = true;
            this.listView1.Location = new System.Drawing.Point(8, 161);
            this.listView1.Name = "listView1";
            this.listView1.Size = new System.Drawing.Size(747, 368);
            this.listView1.TabIndex = 17;
            this.listView1.UseCompatibleStateImageBehavior = false;
            this.listView1.View = System.Windows.Forms.View.Details;
            // 
            // colIndex
            // 
            this.colIndex.Text = "Index";
            this.colIndex.Width = 40;
            // 
            // colDate
            // 
            this.colDate.Text = "Date";
            this.colDate.Width = 50;
            // 
            // colTime
            // 
            this.colTime.Text = "Time";
            this.colTime.Width = 50;
            // 
            // colPhoneNumber
            // 
            this.colPhoneNumber.Text = "Phone Number";
            this.colPhoneNumber.Width = 85;
            // 
            // colCallerID
            // 
            this.colCallerID.Text = "Caller ID";
            this.colCallerID.Width = 120;
            // 
            // colCustomer
            // 
            this.colCustomer.Text = "Customer Name";
            this.colCustomer.Width = 135;
            // 
            // colAddress
            // 
            this.colAddress.Text = "Address";
            this.colAddress.Width = 120;
            // 
            // colCity
            // 
            this.colCity.Text = "City";
            this.colCity.Width = 85;
            // 
            // colState
            // 
            this.colState.Text = "State";
            this.colState.Width = 40;
            // 
            // btPrevRecord
            // 
            this.btPrevRecord.Location = new System.Drawing.Point(54, 109);
            this.btPrevRecord.Name = "btPrevRecord";
            this.btPrevRecord.Size = new System.Drawing.Size(105, 22);
            this.btPrevRecord.TabIndex = 18;
            this.btPrevRecord.Text = "<<";
            this.btPrevRecord.UseVisualStyleBackColor = true;
            this.btPrevRecord.Click += new System.EventHandler(this.btPreRecord_Click);
            // 
            // btNextRecord
            // 
            this.btNextRecord.Location = new System.Drawing.Point(165, 109);
            this.btNextRecord.Name = "btNextRecord";
            this.btNextRecord.Size = new System.Drawing.Size(105, 22);
            this.btNextRecord.TabIndex = 19;
            this.btNextRecord.Text = ">>";
            this.btNextRecord.UseVisualStyleBackColor = true;
            this.btNextRecord.Click += new System.EventHandler(this.btNextRecord_Click);
            // 
            // lbUdpTimeout
            // 
            this.lbUdpTimeout.AutoSize = true;
            this.lbUdpTimeout.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbUdpTimeout.Location = new System.Drawing.Point(58, 88);
            this.lbUdpTimeout.Name = "lbUdpTimeout";
            this.lbUdpTimeout.Size = new System.Drawing.Size(99, 13);
            this.lbUdpTimeout.TabIndex = 20;
            this.lbUdpTimeout.Text = "Waiting for server...";
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.YellowGreen;
            this.ClientSize = new System.Drawing.Size(763, 539);
            this.Controls.Add(this.lbUdpTimeout);
            this.Controls.Add(this.btNextRecord);
            this.Controls.Add(this.btPrevRecord);
            this.Controls.Add(this.listView1);
            this.Controls.Add(this.btnLog);
            this.Controls.Add(this.lbTime);
            this.Controls.Add(this.btClose);
            this.Controls.Add(this.tbState);
            this.Controls.Add(this.tbCity);
            this.Controls.Add(this.pictureBox1);
            this.Controls.Add(this.tbLastInvoice);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.tbAddress);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.lbNumber);
            this.Controls.Add(this.lbName);
            this.Controls.Add(this.tbCustomer);
            this.Controls.Add(this.label2);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.Name = "MainForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Caller ID";
            this.TopMost = true;
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.MainForm_FormClosing);
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.contextMenuStrip1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.NotifyIcon notifyIcon1;
        private System.Windows.Forms.ContextMenuStrip contextMenuStrip1;
        private System.Windows.Forms.ToolStripMenuItem openMenuItem;
        private System.Windows.Forms.ToolStripMenuItem exitMenuItem;
        private System.Windows.Forms.ToolStripMenuItem showMenuItem;
        private System.Windows.Forms.Label lbNumber;
        private System.Windows.Forms.Label lbName;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox tbCustomer;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox tbAddress;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox tbLastInvoice;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.ToolStripMenuItem aboutToolStripMenuItem;
        private System.Windows.Forms.TextBox tbCity;
        private System.Windows.Forms.TextBox tbState;
        private System.Windows.Forms.Button btClose;
        private System.Windows.Forms.Label lbTime;
        private System.Windows.Forms.Button btnLog;
        private System.Windows.Forms.ListView listView1;
        private System.Windows.Forms.ColumnHeader colIndex;
        private System.Windows.Forms.ColumnHeader colDate;
        private System.Windows.Forms.ColumnHeader colTime;
        private System.Windows.Forms.ColumnHeader colPhoneNumber;
        private System.Windows.Forms.ColumnHeader colCallerID;
        private System.Windows.Forms.ColumnHeader colCustomer;
        private System.Windows.Forms.ColumnHeader colAddress;
        private System.Windows.Forms.ColumnHeader colCity;
        private System.Windows.Forms.Button btPrevRecord;
        private System.Windows.Forms.Button btNextRecord;
        private System.Windows.Forms.Label lbUdpTimeout;
        private System.Windows.Forms.ColumnHeader colState;
    }
}

