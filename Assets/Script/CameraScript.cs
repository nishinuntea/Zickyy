/*==============================================================================

    カメラ制御		 [CameraScript.cs]
														 Author : Nishimura
														 Date   : 2026/09/27
--------------------------------------------------------------------------------

==============================================================================*/
using UnityEngine;
using UnityEngine.InputSystem;

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
    [SerializeField] private float minHeight = -1.75f;           // 高さの下限
    [SerializeField] private float maxHeight = 3.0f;             // 高さの上限

    [Header("カメラ回転スピード")]
    [SerializeField] private float rotateSpeed = 35f;                //左右の移動スピード
    [SerializeField] private float heightChangeSpeed = 3.0f;         //上下の移動スピード

    [Header("壁抜け防止（衝突判定）")]
    [SerializeField] private LayerMask collisionLayers;             // カメラが衝突するレイヤー（壁など）
    [SerializeField] private float collisionRadius = 0.2f;          // 当たり判定の球の大きさ

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
        if (enableResetButton && Keyboard.current.rKey.wasPressedThisFrame && playerTarget != null)
        {
            targetCameraAngle = playerTarget.eulerAngles.y;
            height = 15f;
        }

        cameraAngle = Mathf.LerpAngle(cameraAngle, targetCameraAngle, Time.deltaTime * resetSpeed);
    }

    // プレイヤーの足元へ移動し、Y軸だけ回転 ＋ 壁めり込み防止
    private void FollowPlayer()
    {
        if (playerTarget == null) return;

        // 1. 注視点（基準となるターゲット位置）を計算
        Vector3 lookAtPoint = playerTarget.position + new Vector3(0f, lookAtHeight, 0f);

        // 2. 回転角度（Y軸）を設定
        Quaternion rotation = Quaternion.Euler(0f, cameraAngle, 0f);

        // 3. カメラの理想的な最終位置を計算
        Vector3 idealPosition = playerTarget.position
                              + (rotation * Vector3.back * distance)
                              + new Vector3(0f, height, 0f);

        // 4. 注視点から理想位置に向かってスフィアキャストを飛ばし、間に壁がないかチェック
        Vector3 finalPosition = idealPosition;
        Vector3 dirToCamera = idealPosition - lookAtPoint;
        float targetDistance = dirToCamera.magnitude;

        // プレイヤー側からカメラへ向かってレイを飛ばす（壁にぶつかったら手前に押し戻す）
        if (Physics.SphereCast(lookAtPoint, collisionRadius, dirToCamera.normalized, out RaycastHit hit, targetDistance, collisionLayers))
        {
            // 壁に当たった場合、当たった位置よりも少し手前にカメラを配置（埋まり込み防止に少し押し引く）
            finalPosition = hit.point + (hit.normal * collisionRadius);
        }

        // 5. カメラの位置と回転を適用
        transform.position = finalPosition;
        transform.rotation = Quaternion.Euler(0f, cameraAngle, 0f);

        // 6. 常に注視点を向く
        transform.LookAt(lookAtPoint);

        // 7. 画面中央の垂直オフセットを適用
        transform.position += transform.up * screenVerticalOffset;
    }

    // プレイヤーが「今のカメラの水平方向」を取得するための関数
    public Quaternion GetCameraRotation()
    {
        return Quaternion.Euler(0f, transform.eulerAngles.y, 0f);
    }

    // --- 【デバッグ用】カメラがどこを見ているかをSceneビューに緑の球で表示 ---
    private void OnDrawGizmos()
    {
        if (playerTarget != null)
        {
            Vector3 lookAtPoint = playerTarget.position + new Vector3(0f, lookAtHeight, 0f);
            Gizmos.color = Color.green;
            Gizmos.DrawWireSphere(lookAtPoint, 0.3f);
        }
    }
}