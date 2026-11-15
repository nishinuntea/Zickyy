/*==============================================================================

    ジャンプ台		 [JumpPad.cs]
														 Author : Nishimura
														 Date   : 2026/11/15
--------------------------------------------------------------------------------

==============================================================================*/
using UnityEngine;

public class JumpPad : MonoBehaviour
{
    public float jumpHeight = 8f;

    // 一度発動したら、ジャンプ台の外で着地するまで再発動しない
    private bool isLocked = false;
    private CharacterController lockedController;

    void OnTriggerEnter(Collider other)
    {
        if (isLocked) return;

        PlayerControll player = other.GetComponentInParent<PlayerControll>();
        if (player == null) return;

        lockedController = player.GetComponent<CharacterController>();
        isLocked = true;
        player.Launch(jumpHeight);
    }

    void OnTriggerExit(Collider other)
    {
        // ジャンプ台から出た後は、別の場所に着地するまでロックを残す
        // （ここでは解除しない）
    }

    void Update()
    {
        if (!isLocked || lockedController == null) return;

        // ジャンプ台のトリガーの外で接地したらロック解除
        bool isOnGround = lockedController.isGrounded;
        bool insideTrigger = IsInsideTrigger(lockedController);

        if (isOnGround && !insideTrigger)
        {
            isLocked = false;
            lockedController = null;
        }
    }

    bool IsInsideTrigger(CharacterController cc)
    {
        Collider padCol = GetComponent<Collider>();
        if (padCol == null) return false;
        return padCol.bounds.Intersects(cc.bounds);
    }
}