/*==============================================================================

    プレイヤー		 [CameraScript.cs]
														 Author : Nishimura
														 Date   : 2026/09/25
--------------------------------------------------------------------------------

==============================================================================*/
using UnityEngine;
using UnityEngine.InputSystem;

[RequireComponent(typeof(CharacterController))]
public class PlayerControll : MonoBehaviour
{
    [Header("カメラ参照")]
    [SerializeField] private CameraScript cameraScript;

    [Header("プレイヤー設定")]
    [SerializeField] private float moveSpeed = 5.0f;
    [SerializeField] private float playerRotateSpeed = 10f;

    [Header("ジャンプ・重力設定")]
    [SerializeField] private float jumpHeight = 1.5f;     // ジャンプする高さ
    [SerializeField] private float gravity = -19.62f;     // 重力の強さ

    private CharacterController controller;
    private Vector2 moveInput;
    private Vector3 Yvelocity;       // Y軸方向の速度（重力・ジャンプ用）
    private bool isGrounded;

    private void Start()
    {
        controller = GetComponent<CharacterController>();
    }

    // 移動入力
    public void OnMove(InputValue value)
    {
        moveInput = value.Get<Vector2>();
    }

    //ジャンプ入力
    public void OnJump(InputValue value)
    {
        // ボタンが押された瞬間 ＆ 地面に接している時だけジャンプ
        if (value.isPressed && isGrounded)
        {
            // V = √(2 * g * h) の物理公式で指定した高さまで跳べる初速を計算
            Yvelocity.y = Mathf.Sqrt(jumpHeight * -2f * gravity);
        }
    }

    private void Update()
    {
        CheckGrounded();
        MovePlayer();
        ApplyGravity();
    }

    // 地面接地判定
    private void CheckGrounded()
    {
        isGrounded = controller.isGrounded;

        // 地面にいる時は、下向きの速度が溜まりすぎないように固定（坂道での浮き防止）
        if (isGrounded && Yvelocity.y < 0)
        {
            Yvelocity.y = -2f;
        }
    }

    private void MovePlayer()
    {
        if (moveInput.sqrMagnitude < 0.01f) return;

        // CameraScriptから「今のカメラの向き」を取得する
        Quaternion cameraYRotation = Quaternion.identity;
        if (cameraScript != null)
        {
            cameraYRotation = cameraScript.GetCameraRotation();
        }

        // 入力をカメラの向きに合わせた移動方向に変換
        Vector3 moveDirection = cameraYRotation * new Vector3(moveInput.x, 0f, moveInput.y);

        // 進行方向へ振り向く
        Quaternion targetRotation = Quaternion.LookRotation(moveDirection);
        transform.rotation = Quaternion.Slerp(transform.rotation, targetRotation, Time.deltaTime * playerRotateSpeed);

        // 移動を実行
        controller.Move(moveDirection * moveSpeed * Time.deltaTime);
    }

    // 重力・垂直移動処理
    private void ApplyGravity()
    {
        // 毎フレーム重力を加算
        Yvelocity.y += gravity * Time.deltaTime;

        // 垂直（Y軸）移動を実行
        controller.Move(Yvelocity * Time.deltaTime);
    }
}
