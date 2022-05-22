using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CaeControl : MonoBehaviour
{
    private const string HORIZONTAL = "Horizontal";
    private const string VERTICAL = "Vertical";

    private Vector3 startPosition;
    private float horizontalInput;
    private float verticalInput;
    private float currentSteerAngle;
    private float currentbreakForce;
    private bool isBreaking;

    [SerializeField] private float motorForce;
    [SerializeField] private float breakForce;
    [SerializeField] private float maxSteeringAngle;

    [SerializeField] private WheelCollider frontLeftWheelCollider;
    [SerializeField] private WheelCollider frontRightWheelCollider;
    [SerializeField] private WheelCollider rearLeftWheelCollider;
    [SerializeField] private WheelCollider rearRightWheelCollider;

    [SerializeField] private Transform frontLeftWheelTransform;
    [SerializeField] private Transform frontRightWheelTransform;
    [SerializeField] private Transform rearLeftWheelTransform;
    [SerializeField] private Transform rearRightWheelTransform;

    [SerializeField] private Rigidbody carRigidbody;

    void Start()
    {
        /* ��ȡ������ʼλ�� */
        startPosition = transform.position;
    }

    void Update()
    {
        Vector3 pos = carRigidbody.transform.position;
        if (pos[1] < -10)
        {
            /* ʹ����ͣ���� */
            carRigidbody.velocity = Vector3.zero;
            carRigidbody.constraints = RigidbodyConstraints.FreezeAll;
            carRigidbody.constraints = RigidbodyConstraints.None;
            /* �ó��������ǶȺ�ص�ԭ�� */
            carRigidbody.transform.rotation = new Quaternion(0, 0, 0, 0);
            carRigidbody.transform.position = startPosition;
        }

    }

    private void FixedUpdate()
    {
        /* �������� */
        GetInput();         // ��ȡ��������
        HandleMotor();      // ����������ת��Ϊ������
        HanlderSteering();  // ����������ת��Ϊ������
        UpdateWheels();     // �Գ�����ֵ���и���
    }

    private void HandleMotor()
    {
        rearLeftWheelCollider.motorTorque = verticalInput * motorForce;
        rearRightWheelCollider.motorTorque = verticalInput * motorForce;
        currentbreakForce = isBreaking ? breakForce : 0f;
        ApplyBreaking();
    }

    private void ApplyBreaking()
    {
        frontLeftWheelCollider.brakeTorque = currentbreakForce;
        frontRightWheelCollider.brakeTorque = currentbreakForce;
        rearLeftWheelCollider.brakeTorque = currentbreakForce;
        rearRightWheelCollider.brakeTorque = currentbreakForce;
    }

    private void HanlderSteering()
    {
        currentSteerAngle = maxSteeringAngle * horizontalInput;
        frontLeftWheelCollider.steerAngle = currentSteerAngle;
        frontRightWheelCollider.steerAngle = currentSteerAngle;
    }

    private void GetInput()
    {
        horizontalInput = Input.GetAxis(HORIZONTAL); /* �����ϵġ� ���������õ���ֵΪ��-1��1�� */
        verticalInput = Input.GetAxis(VERTICAL);    /* �����ϵġ� ���������õ���ֵΪ��-1��1�� */
        isBreaking = Input.GetKey(KeyCode.Space);
    }

    private void UpdateWheels()
    {
        UpdateSingleWheel(frontLeftWheelCollider, frontLeftWheelTransform);
        UpdateSingleWheel(frontRightWheelCollider, frontRightWheelTransform);
        UpdateSingleWheel(rearLeftWheelCollider, rearLeftWheelTransform);
        UpdateSingleWheel(rearRightWheelCollider, rearRightWheelTransform);
    }

    private void UpdateSingleWheel(WheelCollider wheelCollider, Transform wheelTransform)
    {
        Vector3 pos;
        Quaternion rot;
        wheelCollider.GetWorldPose(out pos, out rot);
        wheelTransform.rotation = rot;
        wheelTransform.position = pos;
    }
}
