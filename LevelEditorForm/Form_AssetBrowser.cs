using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace LevelEditorForm
{
    public partial class AssetBrowserForm : Form
    {
        public AssetBrowserForm()
        {
            InitializeComponent();
        }

        private void fBXToolStripMenuItem_Click(object sender, EventArgs e)
        {
            FBXConverterForm convertForm = new FBXConverterForm();
            convertForm.ShowDialog(this);
        }
    }
}
