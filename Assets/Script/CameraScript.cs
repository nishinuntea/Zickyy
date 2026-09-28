/*==============================================================================

    カメラ制御		 [CameraScript.cs]
														 Author : Nishimura
														 Date   : 2026/09/27
--------------------------------------------------------------------------------

==============================================================================*/
using UnityEngine;
using UnityEngine.InputSystem;

[RequireComponent(typeof(CharacterController))]
public class CameraScript : MonoBehaviour
{
    [Header("プレイヤー")]
    [SerializeField] private Transform playerTarget;        //カメラのターゲット
    [Header("カメラの位置調整")]
    [SerializeField] private float height = 0.0f;    // プレイヤーからの高さ
    [SerializeField] private float distance = 4.0f;  // プレイヤーからの距離

    [Header("カメラ回転スピード")]
    [SerializeField] private float rotateSpeed = 35f;

    private float cameraAngle = 0f;

    private void Start()
    {
        if (playerTarget != null)
        {
            cameraAngle = transform.eulerAngles.y;
        }
    }

    private void Update()
    {
        RotateCamera();
    }

    private void LateUpdate()
    {
        FollowPlayer();
    }

    // 矢印キー（またはQ/E）でカメラの角度を変更
    private void RotateCamera()
    {
        if (Keyboard.current == null) return;

        if (Keyboard.current.leftArrowKey.isPressed || Keyboard.current.qKey.isPressed)
        {
            cameraAngle -= rotateSpeed * Time.deltaTime;
        }
        if (Keyboard.current.rightArrowKey.isPressed || Keyboard.current.eKey.isPressed)
        {
            cameraAngle += rotateSpeed * Time.deltaTime;
        }
    }

    // プレイヤーの足元へ移動し、Y軸だけ回転
    private void FollowPlayer()
    {
        if (playerTarget == null) return;

        // 1. 回転角度（Y軸）を設定
        Quaternion rotation = Quaternion.Euler(0f, cameraAngle, 0f);

        // 2. プレイヤーの位置から「後ろに distance」「上に height」ずらした位置を計算
        // (rotation * Vector3.back で、カメラの回転に合わせた「後ろ方向」を計算)
        Vector3 targetPosition = playerTarget.position
                               + (rotation * Vector3.back * distance)
                               + new Vector3(0f, height, 0f);

        // 3. カメラの位置と回転を適用
        transform.position = targetPosition;
        transform.rotation = Quaternion.Euler(0f, cameraAngle, 0f);

        // 4. 常にプレイヤーの胸/頭あたり（高さオフセットの位置）を注視する
        Vector3 lookAtPoint = playerTarget.position + new Vector3(0f, height * 0.8f, 0f);
        transform.LookAt(lookAtPoint);
    }

    // プレイヤーが「今のカメラの水平方向」を取得するための関数
    public Quaternion GetCameraRotation()
    {
        return Quaternion.Euler(0f, transform.eulerAngles.y, 0f);
    }
}
