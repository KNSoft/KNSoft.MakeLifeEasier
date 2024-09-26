#include "Test.h"

TEST_FUNC(Math_Round)
{
    TEST_OK(Math_RoundUInt(2.7) == 3);
    TEST_OK(Math_RoundInt(2.3) == 2);
    TEST_OK(Math_RoundUIntPtr(2.2f) == 2);
    TEST_OK(Math_RoundIntPtr(2.1) == 2);
    TEST_OK(Math_RoundUInt(1.9) == 2);
    TEST_OK(Math_RoundInt(1.8f) == 2);
    TEST_OK(Math_RoundUIntPtr(1.7) == 2);
    TEST_OK(Math_RoundIntPtr(1.5f) == 2);
    TEST_OK(Math_RoundUInt(1.4) == 1);
    TEST_OK(Math_RoundInt(1.3f) == 1);
    TEST_OK(Math_RoundUIntPtr(1.2f) == 1);
    TEST_OK(Math_RoundIntPtr(1.0f) == 1);
    TEST_OK(Math_RoundUInt(0.9) == 1);
    TEST_OK(Math_RoundInt(0.8f) == 1);
    TEST_OK(Math_RoundUIntPtr(0.7f) == 1);
    TEST_OK(Math_RoundIntPtr(0.6f) == 1);
    TEST_OK(Math_RoundUInt(0.5f) == 1);
    TEST_OK(Math_RoundInt(0.5) == 1);
    TEST_OK(Math_RoundUIntPtr(0.5) == 1);
    TEST_OK(Math_RoundIntPtr(0.5) == 1);
    TEST_OK(Math_RoundUInt(0.4) == 0);
    TEST_OK(Math_RoundInt(0.3) == 0);
    TEST_OK(Math_RoundUIntPtr(0.2) == 0);
    TEST_OK(Math_RoundIntPtr(0.1) == 0);
    TEST_OK(Math_RoundUInt(0) == 0);
    TEST_OK(Math_RoundInt(0.0) == 0);
    TEST_OK(Math_RoundInt(-0.0f) == 0);
    TEST_OK(Math_RoundIntPtr(-0.1f) == -0);
    TEST_OK(Math_RoundInt(-0.2) == 0);
    TEST_OK(Math_RoundIntPtr(-0.3) == 0);
    TEST_OK(Math_RoundInt(-0.4) == -0);
    TEST_OK(Math_RoundIntPtr(-0.5) == -1);
    TEST_OK(Math_RoundInt(-0.5f) == -1);
    TEST_OK(Math_RoundIntPtr(-0.6) == -1);
    TEST_OK(Math_RoundInt(-0.7) == -1);
    TEST_OK(Math_RoundIntPtr(-1.3) == -1);
    TEST_OK(Math_RoundInt(-1.7) == -2);
    TEST_OK(Math_RoundIntPtr(-2.1) == -2);
}
