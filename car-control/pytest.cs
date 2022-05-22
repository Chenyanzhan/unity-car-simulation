using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using System.Diagnostics;

public class pytest : MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {
        RunPythonScript("main.py");
    }

    // Update is called once per frame
    void Update()
    {
        
    }

    public static void RunPythonScript(string sArgName, string args = "")
    {
        Process p = new Process();
        //python�ű���·��
        string path = Application.dataPath + @"/pyScripts/" + sArgName;
        string sArguments = path;


        //(ע�⣺�õĻ���Ҫ�����Լ���)û���价�������Ļ���������������дpython.exe�ľ���·��
        //(�õĻ���Ҫ�����Լ���)��������ˣ�ֱ��д"python.exe"����
        p.StartInfo.FileName = @"F:/software/Anaconda3/envs/pytorch/python.exe";
        //p.StartInfo.FileName = @"C:\Program Files\Python35\python.exe";

        // sArgumentsΪpython�ű���·��   pythonֵ�Ĵ���·��strArr[]->teps->sigstr->sArguments 
        //��python����sys.argv[ ]ʹ�øò���
        p.StartInfo.UseShellExecute = false;
        p.StartInfo.Arguments = sArguments;
        p.StartInfo.RedirectStandardOutput = true;
        p.StartInfo.RedirectStandardInput = true;
        p.StartInfo.RedirectStandardError = true;
        p.StartInfo.CreateNoWindow = true;
        p.Start();
        p.BeginOutputReadLine();
        p.OutputDataReceived += new DataReceivedEventHandler(Out_RecvData);
        //Console.ReadLine();
        //p.WaitForExit();
    }

    static void Out_RecvData(object sender, DataReceivedEventArgs e)
    {
        if (!string.IsNullOrEmpty(e.Data))
        {
            UnityEngine.Debug.Log(e.Data);
        }
    }
}
