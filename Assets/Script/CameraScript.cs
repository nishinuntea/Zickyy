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
    
    [Header("注視点の設定")]
    [SerializeField] private float lookAtHeight = 0.0f;     // 注視点の高さ
  
    [Header("カメラの位置調整")]
    [SerializeField] private float height = 0.0f;           // カメラの高さ
    [SerializeField] private float distance = 2.0f;         // カメラの距離
    [SerializeField] private float screenVerticalOffset = -0.3f;

    [Header("カメラ高さ制限（上限・下限）")]
    [SerializeField] private float minHeight = -1.75f;          // 高さの下限
    [SerializeField] private float maxHeight = 3.0f;            // 高さの上限

    [Header("カメラ回転スピード")]
    [SerializeField] private float rotateSpeed = 35f;               //左右の移動スピード
    [SerializeField] private float heightChangeSpeed = 3.0f;        //上下の移動スピード

    [Header("自動カメラ機能")]
    [SerializeField] private bool autoRotateWithPlayer = true;      // 移動時にカメラが自動追従
    [SerializeField] private bool enableResetButton = true;         // カメラリセット
 
    //内部変数
    private float cameraAngle = 0f;
    private float targetCameraAngle = 0f;
    private float autoRotateSpeed = 2.0f;       // 自動追従のスピード
    private float resetSpeed = 20.0f;           // カメラリセットのスピード

    private void Start()
    {
        if (playerTarget != null)
        {
            cameraAngle = transform.eulerAngles.y;
            targetCameraAngle = cameraAngle;
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

        // 左右旋回
        if (Keyboard.current.qKey.isPressed || Keyboard.current.leftArrowKey.isPressed)
        {
            targetCameraAngle -= rotateSpeed * Time.deltaTime;
        }
        if (Keyboard.current.eKey.isPressed || Keyboard.current.rightArrowKey.isPressed)
        {
            targetCameraAngle += rotateSpeed * Time.deltaTime;
        }

        // 上下
        if (Keyboard.current.upArrowKey.isPressed)
        {
            height += heightChangeSpeed * Time.deltaTime;
        }
        if (Keyboard.current.downArrowKey.isPressed)
        {
            height -= heightChangeSpeed * Time.deltaTime;
        }

        height = Mathf.Clamp(height, minHeight, maxHeight);

        // Rで背面リセット
        if (enableResetButton &&Keyboard.current.rKey.wasPressedThisFrame &&playerTarget != null)
        {
            targetCameraAngle = playerTarget.eulerAngles.y;
            height = 15f;
        }

        cameraAngle = Mathf.LerpAngle(cameraAngle, targetCameraAngle, Time.deltaTime * resetSpeed);
    }

    // プレイヤーの足元へ移動し、Y軸だけ回転
    private void FollowPlayer()
    {
        if (playerTarget == null) return;

        // 1. 回転角度（Y軸）を設定
        Quaternion rotation = Quaternion.Euler(0f, cameraAngle, 0f);

        // 2. プレイヤーの位置から「後ろに distance」「上に height」ずらした位置を計算
        Vector3 targetPosition = playerTarget.position
                               + (rotation * Vector3.back * distance)
                               + new Vector3(0f, height, 0f);

        // 3. カメラの位置と回転を適用
        transform.position = targetPosition;
        transform.rotation = Quaternion.Euler(0f, cameraAngle, 0f);

        // 4. 常にプレイヤーの lookAtHeight を注視する
        Vector3 lookAtPoint = playerTarget.position + new Vector3(0f, lookAtHeight, 0f);
        transform.LookAt(lookAtPoint);

        // 5. 画面中央を「照準（レティクル）」にするため、プレイヤーの位置を画面中央より少し下へオフセット
        transform.position += transform.up * screenVerticalOffset;
    }

    // プレイヤーが「今のカメラの水平方向」を取得するための関数
    public Quaternion GetCameraRotation()
    {
        return Quaternion.Euler(0f, transform.eulerAngles.y, 0f);
    }
}
