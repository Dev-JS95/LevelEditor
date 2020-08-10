using LevelEditorWrap;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace LevelEditorForm
{

    public partial class FBXConverterForm : Form
    {
        LevelEditorWrapper editor;

        //렌더링 쓰레드
        Thread RenderThread;
        bool CanRendering = true;   //렌더링 쓰레드 실행 가능한지 체크

        public FBXConverterForm()
        {
            InitializeComponent();

            try
            {
                //에디터 생성
                editor = new LevelEditorWrapper(P_Viewport.Handle);
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());

                //정리
                Close();
            }

            //게임 루프 연결
            RenderThread = new Thread(GameLoop);
            RenderThread.IsBackground = true;
            RenderThread.Start();
        }

        //게임 루프
        void GameLoop()
        {
            while (true)
            {
                if (CanRendering)
                {
                    editor.Tick();
                }
                else
                {
                    System.Threading.Thread.Sleep(300);
                }
            }
        }

        private void openToolStripMenuItem_Click(object sender, EventArgs e)
        {
            OpenFileDialog openFile = new OpenFileDialog();
            openFile.DefaultExt = "fbx";
            openFile.Filter = "FBX Files(*.fbx)|*.fbx; *.FBX;";

            if (openFile.ShowDialog() == DialogResult.OK)
            {
                if (openFile.FileNames.Length > 0)
                {
                    //this.L_FileName.Text = openFile.SafeFileName;

                    //editor에서 FBXConvert 메서드 실행

                }
            }
        }

        private void FBXConverterForm_Activated(object sender, EventArgs e)
        {
            CanRendering = true;

        }

        private void FBXConverterForm_Deactivate(object sender, EventArgs e)
        {
            CanRendering = false;

        }
    }
}
