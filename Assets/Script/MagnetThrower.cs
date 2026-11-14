/*==============================================================================

    磁石を投げる		 [MagnetThrower.cs]
														 Author : Nishimura
														 Date   : 2026/11/14
--------------------------------------------------------------------------------

==============================================================================*/
using UnityEngine;
using UnityEngine.InputSystem;

public class MagnetThrower : MonoBehaviour 
{
    [Header("Settings")]
    public GameObject magnetPrefab;
    public Transform throwPoint;
    public LineRenderer lineRenderer;

    [Header("Throw Parameters")]
    public float throwPower = 15f;
    public int lineSegments = 30;
    public float timeStep = 0.05f;

    private bool isAiming = false;
    private bool wasFKeyPressedLastFrame = false;

    void Update()
    {
        // デフォルトのInput Systemからキーボードの状態を取得
        var keyboard = Keyboard.current;
        if (keyboard == null) return;

        // Fキーの押下状態を取得
        bool isFKeyPressed = keyboard.fKey.isPressed;
        bool isFKeyReleased = keyboard.fKey.wasReleasedThisFrame;

        // Fキーを押した瞬間（エイム開始）
        if (isFKeyPressed && !wasFKeyPressedLastFrame)
        {
            isAiming = true;
        }

        // エイム中（Fキー長押し中）は放物線の軌道を描画し続ける
        if (isAiming)
        {
            DrawTrajectory();
        }

        // Fキーを離した瞬間（投擲）
        if (isAiming && isFKeyReleased)
        {
            isAiming = false;
            if (lineRenderer != null) lineRenderer.positionCount = 0;
            ThrowMagnetAction();
        }

        wasFKeyPressedLastFrame = isFKeyPressed;
    }

    void DrawTrajectory()
    {
        if (lineRenderer == null || throwPoint == null) return;

        lineRenderer.positionCount = lineSegments;
        Vector3 startPos = throwPoint.position;
        Vector3 startVelocity = throwPoint.forward * throwPower;

        Vector3[] points = new Vector3[lineSegments];
        for (int i = 0; i < lineSegments; i++)
        {
            float t = i * timeStep;
            Vector3 point = startPos + startVelocity * t + 0.5f * Physics.gravity * t * t;
            points[i] = point;

            if (i > 0 && Physics.Linecast(points[i - 1], point, out RaycastHit hit))
            {
                lineRenderer.positionCount = i + 1;
                points[i] = hit.point;
                break;
            }
        }
        lineRenderer.SetPositions(points);
    }

    void ThrowMagnetAction()
    {
        if (magnetPrefab == null || throwPoint == null) return;

        GameObject magnet = Instantiate(magnetPrefab, throwPoint.position, throwPoint.rotation);
        Rigidbody rb = magnet.GetComponent<Rigidbody>();
        if (rb != null)
        {
            // Unity 6のRigidbodyでは linearVelocity を使用
            rb.linearVelocity = throwPoint.forward * throwPower;
        }
    }
}
